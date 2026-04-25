#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include "../process/processs.h"
#include "../memory/memoryy.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../os/os_core.h"

// Link to the GUI logger
extern void gui_log(const char* format, ...);
 
Process *execute_hrrn() {
   
    print_all_queues();
 
    Process *current = os_get_running_process();
    if (current == NULL) {
        if (is_empty(&os_ready_queue)) {
            gui_log("Scheduler: no proccesses in the ready queue");
            return NULL; 
        }
        
        QueueNode *node = os_ready_queue.head;
        Process *best = NULL;
        double myrate = -1.0;
        
        // response ratio = (wait + burst) / burst.
        while (node != NULL) {
            Process *p = node->process;
 
            int wait = p->wait_time + (os_get_clock() - p->ready_since);
            if (wait < 0) wait = 0;
            int burst = p->code_line_count;
            if (burst < 1) burst = 1; 
            
            double ratio = (double)(wait + burst) / burst;
            gui_log("  P%d: wait=%d  burst=%d  ratio=%.2f", p->pcb->pid, wait, burst, ratio);
 
            if (ratio > myrate) {
                myrate = ratio;
                best = p;
            }
            node = node->next;
        }
 
        //Remove the selected process and assign it to the CPU
 
        remove_from_queue(&os_ready_queue, best);
        current = best;
 
        //Persist accumulated waiting time up to this point.
        current->wait_time += os_get_clock() - current->ready_since;
        if (current->wait_time < 0) current->wait_time = 0;
 
        gui_log("HRRN: Selected Process %d (Ratio: %.2f)", current->pcb->pid, myrate);
        print_all_queues(); 
    }
 
    //Execute EXACTLY ONE instruction (Matches 1 OS clock tick)
    gui_log("Running Process %d | PC: %d", current->pcb->pid, current->pcb->pc);
    swap_in(current->pcb->pid);
    sync_pcb_from_memory(current->pcb->pid, current->pcb);
    current->pcb->state = RUNNING;
    update_state_in_memory(current->pcb->pid, RUNNING);
    scheduler_set_last_executed_pid(current->pcb->pid); //gui

    print_memory();
    execute_instruction(current);

    if (instruction_stalled_on_input()) {
        print_all_queues();
        return current;
    }
 
    //Handle State Changes
    if (current->pcb->state == FINISHED) {
        gui_log("Process %d has finished.", current->pcb->pid);
        update_state_in_memory(current->pcb->pid, FINISHED);
        print_all_queues(); 
        return current; 
    }    
    
    if (current->pcb->state == BLOCKED) {
       gui_log("Process %d is blocked", current->pcb->pid);
       update_state_in_memory(current->pcb->pid, BLOCKED);
       print_all_queues(); 
       
       // CRITICAL: Return NULL so os_step knows the CPU is free! 
       // If you return `current` here, the OS will get stuck trying to run a blocked process.
       return NULL; 
    }
 
    //Process is still RUNNING. Return it so os_step keeps it on the CPU next tick.
    return current;
}
 
//old hrrn may be needed
 
// #include "scheduler.h"
// #include "../interpreter/interpreter.h"
// #include "../process/processs.h"
// #include "../memory/memoryy.h"
// #include "queue.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include "../os/os_core.h"
 
// extern int os_get_clock(void);
 
// Process *execute_hrrn(){
//     // PCB *current_process = dequeue(&os_ready_queue);
//     printf("Executing Highest Respose Rate Algorithm");
//     if (os_ready_queue.head == NULL) {
//         printf("Scheduler: no proccesses in the ready queue\n");
//         return NULL; // no processes ready to run
//     }
    
//     QueueNode *current = os_ready_queue.head;
   
//     Process * best =NULL;
//     double myrate = -1.0;
    
 
//     // pick the chosen process based on response ratio using global system clock.
//     int wait;
//     // while (current != NULL) {
//     //     Process *p=current->process;
//     //     wait = os_get_clock() - p->arrival_time;
//     //     int current_wait = p->wait_time + (os_get_clock() - p->ready_since);
//     //     //double ratio = (double)(current_wait + burst) / burst;
//     //     int burst = (p->code_line_count + time_quantum - 1) / time_quantum;
//     //     if (burst < 1) burst = 1;
        
//     //     double ratio = (double)(current_wait + burst) / burst;
//     //     printf("  P%d: arrival=%d  wait=%d  burst=%d  ratio=%.2f\n",p->pcb->pid, p->arrival_time, wait, burst, ratio);
 
//     //     if(ratio>myrate) {
//     //         myrate = ratio;
//     //         best = p;
//     //     }
//     //     current = current->next;
//     // }
 
 
 
 
 
//     // pick the chosen process based on response ratio.
//     while (current != NULL) {
//         Process *p = current->process;
        
//         // 1. Use the wait_time you are manually incrementing at the bottom of the file
//         int current_wait = p->wait_time; 
        
//         // 2. Fix Burst Time: HRRN uses total instructions, do NOT divide by time_quantum
//         int burst = p->code_line_count;
//         if (burst < 1) burst = 1; // Safety check to prevent division by zero
        
//         // 3. Calculate Ratio
//         double ratio = (double)(current_wait + burst) / burst;
        
//         // 4. Print the exact variables you are using for the math
//         printf("  P%d: arrival=%d  wait=%d  burst=%d  ratio=%.2f\n", 
//                 p->pcb->pid, p->arrival_time, current_wait, burst, ratio);
 
//         if(ratio > myrate) {
//             myrate = ratio;
//             best = p;
//         }
//         current = current->next;
//     }
 
//     //Remove the selected process from the middle of the queue
//     remove_from_queue(&os_ready_queue, best);
//     best->wait_time += os_get_clock() - best->ready_since;
//     best->pcb->state = RUNNING;
//     update_state_in_memory(best->pcb->pid, RUNNING);
//     printf("HRRN: Selected Process %d (Ratio: %.2f)\n", best->pcb->pid, myrate);
//     print_all_queues(); 
 
//     // run until it finishes or blocks
//     int inst_count=0;
//     while (best->pcb->state == RUNNING) {
//         printf("Running Process %d | PC: %d\n", best->pcb->pid, best->pcb->pc);
//         execute_instruction(best);
//         inst_count++;
 
//         if (best->pcb->state == FINISHED) {
//             printf("Process %d has finished.\n", best->pcb->pid);
//             update_state_in_memory(best->pcb->pid, FINISHED);
//             print_all_queues(); 
//             break;
//         }    
//         if (best->pcb->state == BLOCKED) {
//            printf("Process %d is blocked\n", best->pcb->pid);
//            update_state_in_memory(best->pcb->pid, BLOCKED);
//            print_all_queues(); 
//            break;
//         }
//     }
//     QueueNode *curr= os_ready_queue.head;
//     while(curr!=NULL){
//         curr->process->wait_time += inst_count;
//         curr=curr->next;
//     }
 
//     return best;
// }
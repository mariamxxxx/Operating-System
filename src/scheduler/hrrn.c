#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include "../process/processs.h"
#include "../memory/memoryy.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include "../os/os_core.h"

extern int os_get_clock(void);

Process *execute_hrrn(){
    // PCB *current_process = dequeue(&os_ready_queue);
    printf("Executing Highest Respose Rate Algorithm");
    if (os_ready_queue.head == NULL) {
        printf("Scheduler: no proccesses in the ready queue\n");
        return NULL; // no processes ready to run
    }
    
    QueueNode *current = os_ready_queue.head;
   
    Process * best =NULL;
    double myrate = -1.0;
    

    // pick the chosen process based on response ratio using global system clock.
    int wait;
    while (current != NULL) {
        Process *p=current->process;
        wait = os_get_clock() - p->arrival_time;
        int current_wait = p->wait_time + (os_get_clock() - p->ready_since);
        //double ratio = (double)(current_wait + burst) / burst;
        int burst = (p->code_line_count + time_quantum - 1) / time_quantum;
        if (burst < 1) burst = 1;
        
        double ratio = (double)(current_wait + burst) / burst;
        printf("  P%d: arrival=%d  wait=%d  burst=%d  ratio=%.2f\n",p->pcb->pid, p->arrival_time, wait, burst, ratio);

        if(ratio>myrate) {
            myrate = ratio;
            best = p;
        }
        current = current->next;
    }

    //Remove the selected process from the middle of the queue
    remove_from_queue(&os_ready_queue, best);
    best->wait_time += os_get_clock() - best->ready_since;
    best->pcb->state = RUNNING;
    update_state_in_memory(best->pcb->pid, RUNNING);
    printf("HRRN: Selected Process %d (Ratio: %.2f)\n", best->pcb->pid, myrate);
    print_all_queues(); 

    // run until it finishes or blocks
    int inst_count=0;
    while (best->pcb->state == RUNNING) {
        printf("Running Process %d | PC: %d\n", best->pcb->pid, best->pcb->pc);
        execute_instruction(best);
        inst_count++;

        if (best->pcb->state == FINISHED) {
            printf("Process %d has finished.\n", best->pcb->pid);
            update_state_in_memory(best->pcb->pid, FINISHED);
            print_all_queues(); 
            break;
        }    
        if (best->pcb->state == BLOCKED) {
           printf("Process %d is blocked\n", best->pcb->pid);
           update_state_in_memory(best->pcb->pid, BLOCKED);
           print_all_queues(); 
           break;
        }
    }
    QueueNode *curr= os_ready_queue.head;
    while(curr!=NULL){
        curr->process->wait_time += inst_count;
        curr=curr->next;
    }

    return best;
}

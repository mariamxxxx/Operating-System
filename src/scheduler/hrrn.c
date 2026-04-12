#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include "../process/pcb.h"
#include "../process/processs.h"
#include <stdio.h>
PCB*execute_hrrn(){
    // PCB *current_process = dequeue(&os_ready_queue);
    printf("Executing Highest Respose Rate Algorithm");
    if (os_ready_queue.head == NULL) {
        printf("Scheduler: no proccesses in the ready queue\n");
        return NULL; // no processes ready to run
    }
    
    QueueNode *current = os_ready_queue.head;
   
    //PCB *best_pcb = NULL;//the chosen process
    Process * best =NULL;
    double myrate = -1.0;
    

    //pick the chosen process 
    while (current != NULL) {
        Process *p=
        int burst_time= p->code_line_count/ time_quantum;
        double ratio = (double)(p->wait_time + burst_time) / burst_time;
        if (ratio > myrate) {
            myrate = ratio;
            best_pcb = p;
        }
        current = current->next;
    }

    //Remove the selected process from the middle of the queue
    remove_from_queue(&os_ready_queue, best_pcb);
 
    best_pcb->state = RUNNING;
    update_state_in_memory(best_pcb-> pid, RUNNING);
    printf("HRRN: Selected Process %d (Ratio: %.2f)\n", best_pcb->pid, myrate);
    print_all_queues(); 

    // run until it finishes or blocks
    int inst_count=0;
    while (best_pcb->state == RUNNING) {
        printf("Running Process %d | PC: %d\n", best_pcb->pid, best_pcb->pc);
        execute_instruction(best_pcb); //form heba
        inst_count++;

        if (best_pcb->state == FINISHED) {
            printf("Process %d has finished.\n", best_pcb->pid);
            update_state_in_memory(best_pcb-> pid, FINISHED);
            print_all_queues(); 
            break;
        }    
        if (best_pcb->state == BLOCKED) {
           printf("Process %d is blocked\n", best_pcb->pid);
           update_state_in_memory(best_pcb-> pid, BLOCKED);
           print_all_queues(); 
           break;
        }
    }
    QueueNode *curr= os_ready_queue.head;
    while(curr!=NULL){
        curr->process->wait_time+= inst_count;
        curr=curr->next;
    }

    return best_pcb;
}

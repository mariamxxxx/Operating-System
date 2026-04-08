#include "scheduler.h"
#include "../interpreter/interpreter.h" //interpreter from heba
#include <stdio.h>
PCB*execute_hrrn(){
    PCB *current_process = dequeue(&os_ready_queue);
    if (current_process == NULL) {
        printf("Scheduler: no proccesses in the ready queue\n");
        return NULL; // no processes ready to run
    }
    QueueNode *current = os_ready_queue.head;
    PCB *best_pcb = NULL;//the chosen process
    double myrate = -1.0;

    while (current != NULL) {
        PCB *p = current->process;
        
        double ratio = (double)(p->wait_time + p->burst_time) / p->burst_time;
        
        if (ratio > myrate) {
            myrate = ratio;
            best_pcb = p;
        }
        current = current->next;
    }

    // Remove the selected process from the middle of the queue
    remove_from_queue(&os_ready_queue, best_pcb);
 
    best_pcb->state = RUNNING;
    printf("HRRN: Selected Process %d (Ratio: %.2f)\n", best_pcb->pid, myrate);
    print_all_queues(); 

    // run until it finishes or blocks
    while (best_pcb->state == RUNNING) {
        printf("Running Process %d | PC: %d\n", best_pcb->pid, best_pcb->pc);
        
        execute_instruction(best_pcb); //form heba

        if (best_pcb->state == FINISHED) {
            printf("Process %d has finished.\n", best_pcb->pid);
            print_all_queues(); 
            break;
        }    
        if (best_pcb->state == BLOCKED) {
           printf("Process %d is blocked\n", best_pcb->pid);
           print_all_queues(); 
           break;
        }
    }

    return best_pcb;
}

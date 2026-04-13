#include "scheduler.h"
#include <stdio.h>
#include <stddef.h> //for null pointer 
#include "../memory/memoryy.h"
#include "../interpreter/interpreter.h"

//the global queues for the whole OS
 Queue os_ready_queue;
 Queue general_blocked_queue; 
 int time_quantum = 2;

static SchedulerAlgorithm current_algo;

void init_scheduler() {
    init_queue(&os_ready_queue);
    init_queue(&general_blocked_queue);
    init_mlfq();
} //will be called in the main implemetation


void print_all_queues() {
    printf("\n--- SYSTEM QUEUE STATUS ---\n");

    // Ready Queue
    printf("Ready Queue: [");
    QueueNode *current = os_ready_queue.head;
    while (current != NULL) {
        printf("P%d", current->process->pcb->pid);
        if (current->next != NULL) printf(", ");
        current = current->next;
    }
    printf("]\n");

    // General Blocked Queue
    printf("Blocked Queue: [");
    current = general_blocked_queue.head;
    while (current != NULL) {
        printf("P%d", current->process->pcb->pid);
        if (current->next != NULL) printf(", ");
        current = current->next;
    }
    printf("]\n");

    //for mlfq

    if(get_current_algo()==MLFQ){
    for (int i = 0; i < 4; i++) {
    printf("MLFQ Queue %d: [", i);
    QueueNode *curr = mlfq_queues[i].head;
    while (curr != NULL) {
        printf("P%d", curr->process->pcb->pid);
        if (curr->next != NULL) printf(", ");
        curr = curr->next;
    }
    printf("]\n");
}
}
}


void add_process_to_scheduler(Process *process) {
    if (process == NULL || process->pcb == NULL) {
        return;
    }

    process->pcb->state = READY;
    process->ready_since= os_get_clock();
    if (current_algo == MLFQ) {
        enqueue(process, &mlfq_queues[0]); // new processes start at highest priority
    } else {
        enqueue(process, &os_ready_queue);
    }

    
}

Process* schedule_next_process(SchedulerAlgorithm algo) {
    current_algo=algo;
    switch(algo) {
        case RR:   return execute_round_robin();
        case HRRN: return execute_hrrn();
        case MLFQ: return execute_mlfq();
        default: return NULL;
    }
}

SchedulerAlgorithm get_current_algo() {
    return current_algo;
}

void set_current_algo(SchedulerAlgorithm algo) {
    current_algo = algo;
}
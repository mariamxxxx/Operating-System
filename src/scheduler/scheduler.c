#include "scheduler.h"
#include <stddef.h> // for null pointer 

// the global queue for the whole OS
ReadyQueue os_ready_queue;

void init_scheduler() {
    init_queue(&os_ready_queue);
}

PCB* schedule_next_process(SchedulerAlgorithm algo) {
    if (algo == RR) {
        return execute_round_robin();
    } else if (algo == HRRN) {
        return execute_hrrn();
    }
    return NULL; 
}
#include "scheduler.h"
#include <stdio.h>

// Assuming your friend's blocked queues are accessible 
// and use the same QueueNode structure.
extern ReadyQueue os_ready_queue;
extern ReadyQueue general_blocked_queue; 

void print_all_queues() {
    printf("\n--- SYSTEM QUEUE STATUS ---\n");

    // Ready Queue
    printf("Ready Queue: [");
    QueueNode *current = os_ready_queue.head;
    while (current != NULL) {
        printf("P%d", current->process->pid);
        if (current->next != NULL) printf(", ");
        current = current->next;
    }
    printf("]\n");

    // General Blocked Queue
    printf("Blocked Queue: [");
    current = general_blocked_queue.head;
    while (current != NULL) {
        printf("P%d", current->process->pid);
        if (current->next != NULL) printf(", ");
        current = current->next;
    }
    printf("]\n");
    //printf("---------------------------\n\n");
}

PCB* schedule_next_process(SchedulerAlgorithm algo) {
    switch(algo) {
        case RR:   return execute_round_robin();
        case HRRN: return execute_hrrn();
        case MLFQ: return execute_mlfq();
        default: return NULL;
    }
}
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
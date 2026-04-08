#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "ready_queue.h"

typedef enum{
    HRRN,
    RR,
    MLFQ
}SchedulerAlgorithm;

extern ReadyQueue os_ready_queue; //the main ready queue that holds the proccesses


void init_scheduler();
void add_process_to_scheduler(PCB *process);
void print_all_queues();
// the master function the OS will call every time it needs a new process
PCB* schedule_next_process(SchedulerAlgorithm algo);

// the specific algorithms in rr.c, hrrn.c, and mlfq.c
extern ReadyQueue mlfq_queues[4];

void init_scheduler();
PCB* execute_mlfq();
PCB* execute_round_robin();
PCB* execute_hrrn();
//PCB* execute_mlfq(ReadyQueue queues[4]);

#endif
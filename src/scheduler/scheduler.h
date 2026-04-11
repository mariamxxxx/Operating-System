#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

typedef enum{
    HRRN,
    RR,
    MLFQ
}SchedulerAlgorithm;

extern Queue os_ready_queue; //the main ready queue that holds the proccesses
extern Queue general_blocked_queue;
extern Queue mlfq_queues[4];

void init_mlfq();
void init_scheduler();
void add_process_to_scheduler(PCB *process);
void print_all_queues();
SchedulerAlgorithm get_current_algo(); 

// the master function the OS will call every time it needs a new process
PCB* schedule_next_process(SchedulerAlgorithm algo);


PCB* execute_mlfq();
PCB* execute_round_robin();
PCB* execute_hrrn();
//PCB* execute_mlfq(ReadyQueue queues[4]);

#endif
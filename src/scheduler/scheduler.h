#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"
#include "../process/processs.h"

typedef enum{
    HRRN,
    RR,
    MLFQ
}SchedulerAlgorithm;

extern Queue os_ready_queue; //the main ready queue that holds the proccesses
extern Queue general_blocked_queue;
extern Queue mlfq_queues[4];
extern int time_quantum;

void init_mlfq();
void init_scheduler();
void set_current_algo(SchedulerAlgorithm algo);
void add_process_to_scheduler(Process* process);
void set_pending_rr_process(Process *process);
void flush_pending_rr_process(void);
void print_all_queues();
SchedulerAlgorithm get_current_algo(); 
void scheduler_set_last_executed_pid(int pid); //gui
int scheduler_get_last_executed_pid(void); //gui

// the master function the OS will call every time it needs a new process
Process* schedule_next_process(SchedulerAlgorithm algo);


Process* execute_mlfq();
Process* execute_round_robin();
Process* execute_hrrn();
//PCB* execute_mlfq(ReadyQueue queues[4]);

#endif
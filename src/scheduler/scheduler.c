#include "scheduler.h"
#include <stdio.h>
#include <stddef.h> //for null pointer 
#include <stdarg.h>
#include "../memory/memoryy.h"
#include "../interpreter/interpreter.h"
#include "../os/os_core.h"

#ifdef GUI_MODE
// Link to the GUI logger
extern void gui_log(const char *format, ...);
#define LOGF(...) gui_log(__VA_ARGS__)
#else
#define LOGF(...) printf(__VA_ARGS__)
#endif

//the global queues for the whole OS
 Queue os_ready_queue;
 Queue general_blocked_queue; 
 int time_quantum = 2;

static SchedulerAlgorithm current_algo;
static Process *pending_rr_process = NULL;
static int g_scheduler_last_executed_pid = -1; //gui

void init_scheduler() {
    init_queue(&os_ready_queue);
    init_queue(&general_blocked_queue);
    init_mlfq();
    pending_rr_process = NULL;
    g_scheduler_last_executed_pid = -1; //gui
} //will be called in the main implemetation


void print_all_queues() {
    LOGF("--- SYSTEM QUEUE STATUS ---");

    char buffer[512];
    int pos = 0;

    // Ready Queue
    pos = snprintf(buffer, sizeof(buffer), "Ready Queue: [");
    QueueNode *current = os_ready_queue.head;
    while (current != NULL) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "P%d", current->process->pcb->pid);
        if (current->next != NULL) pos += snprintf(buffer + pos, sizeof(buffer) - pos, ", ");
        current = current->next;
    }
    snprintf(buffer + pos, sizeof(buffer) - pos, "]");
    LOGF("%s", buffer);

    // General Blocked Queue
    pos = snprintf(buffer, sizeof(buffer), "Blocked Queue: [");
    current = general_blocked_queue.head;
    while (current != NULL) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "P%d", current->process->pcb->pid);
        if (current->next != NULL) pos += snprintf(buffer + pos, sizeof(buffer) - pos, ", ");
        current = current->next;
    }
    snprintf(buffer + pos, sizeof(buffer) - pos, "]");
    LOGF("%s", buffer);

    //for mlfq
    if(get_current_algo() == MLFQ){
        for (int i = 0; i < 4; i++) {
            pos = snprintf(buffer, sizeof(buffer), "MLFQ Queue %d: [", i);
            QueueNode *curr = mlfq_queues[i].head;
            while (curr != NULL) {
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "P%d", curr->process->pcb->pid);
                if (curr->next != NULL) pos += snprintf(buffer + pos, sizeof(buffer) - pos, ", ");
                curr = curr->next;
            }
            snprintf(buffer + pos, sizeof(buffer) - pos, "]");
            LOGF("%s", buffer);
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
    // Idle steps must not see a stale "input stall" (would freeze the sim clock).
    instruction_clear_stall();
    current_algo=algo;
    g_scheduler_last_executed_pid = -1; //gui
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

void set_pending_rr_process(Process *process) {
    pending_rr_process = process;
}

void flush_pending_rr_process(void) {
    if (pending_rr_process == NULL) {
        return;
    }

    enqueue(pending_rr_process, &os_ready_queue);
    pending_rr_process = NULL;
}

void scheduler_set_last_executed_pid(int pid) {
    g_scheduler_last_executed_pid = pid;
} //gui

int scheduler_get_last_executed_pid(void) {
    return g_scheduler_last_executed_pid;
} //gui
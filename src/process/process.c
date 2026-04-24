#include "processs.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


int next_pid = 1; // Start IDs at 1

Process* create_process(int mem_start, int mem_end, int burst_time) {
    Process *process = (Process *) malloc(sizeof(Process));

    if (process == NULL) return NULL;

    process->pcb = (PCB *) malloc(sizeof(PCB));
    if (process->pcb == NULL) {
        free(process);
        return NULL;
    }

    process->pcb->pid = next_pid++;
    process->pcb->state = READY;
    process->pcb->pc = mem_start;
    process->pcb->memory_bounds[0] = mem_start;
    process->pcb->memory_bounds[1] = mem_end;

    process->var1 = NULL;
    process->var2 = NULL;
    process->var3 = NULL;
    process->code_line_count = burst_time;
    process->arrival_time = 0;
    process->wait_time = 0;
    process->ready_since = 0;
    memset(process->code_lines, 0, sizeof(process->code_lines));

    return process;
}

void destroy_process(Process* process) {
    if (process != NULL) {
        free(process->pcb);
        free(process);
    }
}

PCB* initPCB(int pid) {
    PCB* pcb = (PCB*) malloc(sizeof(PCB));
    if (pcb == NULL) {
        return NULL;
    }

    pcb->pid = pid;
    pcb->state = NEW;
    pcb->pc = -1;
    pcb->memory_bounds[0] = -1;
    pcb->memory_bounds[1] = -1;

    return pcb;
}

Process* initProcess(int arrival_time) {

    Process* process = (Process*) malloc(sizeof(Process));
    if (process == NULL) {
        return NULL;
    }

    process->pcb = initPCB(next_pid++);
    if (process->pcb == NULL) {
        free(process);
        return NULL;
    }

    process->var1 = NULL;
    process->var2 = NULL;
    process->var3 = NULL;

    process->code_line_count = 0;
    process->arrival_time = arrival_time;

    process->wait_time = 0;
    process->ready_since= 0;

    // // Clear code memory
    // for (int i = 0; i < MAX_CODE_LINES; i++) {
    //     memset(process->code_lines[i], '\0', MAX_STRING);
    // }

    return process;
}
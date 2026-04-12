#include "processs.h"
#include <stdlib.h>

int next_pid = 1; // Start IDs at 1

PCB* create_process(int mem_start, int mem_end, int burst_time) {
    PCB* new_pcb = (PCB*)malloc(sizeof(PCB));
    
    if (new_pcb == NULL) return NULL;

    //assign unique ID
    new_pcb->pid = next_pid++;

    //set initial state
    new_pcb->state = READY;

    // initialize Program Counter to the start of memory 
    new_pcb->pc = mem_start;

    //mem boundries
    new_pcb->memory_bounds[0] = mem_start;
    new_pcb->memory_bounds[1] = mem_end;

    //initialize Scheduler fields
    new_pcb->burst_time = burst_time;
    new_pcb->wait_time = 0;

    return new_pcb;
}

void destroy_process(PCB* p) {
    if (p != NULL) {
        free(p);
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

    pcb->burst_time = 0;
    pcb->wait_time  = 0;

    return pcb;
}

Process* initProcess(int pid, int lines_of_code, int arrival_time) {
    Process* process = (Process*) malloc(sizeof(Process));
    if (process == NULL) {
        return NULL;
    }

    process->pcb = initPCB(pid);
    if (process->pcb == NULL) {
        free(process);
        return NULL;
    }

    process->var1 = NULL;
    process->var2 = NULL;
    process->var3 = NULL;

    process->code_line_count = lines_of_code;
    process->arrival_time = arrival_time;


    // // Clear code memory
    // for (int i = 0; i < MAX_CODE_LINES; i++) {
    //     memset(process->code_lines[i], '\0', MAX_STRING);
    // }

    return process;
}
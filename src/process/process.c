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
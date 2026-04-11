#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process/pcb.h"
#include "scheduler/scheduler.h"
#include "synchronization/mutex.h"
#include "interpreter/interpreter.h"
#include "memory/memoryy.h"
#include "memory/memory.c"
#include "process/processs.h"



void controller(FILE *input_file, int arrival_time){
    // 1. Read and interpret the input file to create a Process structure
    loadAndInterpret(input_file);
    if (proc == NULL) {
        fprintf(stderr, "Failed to load process from file.\n");
        return;
    }

    // 2. Allocate memory for the process
    int mem_index = allocate_memory(proc->pcb->pid, proc);
    if (mem_index == -1) {
        fprintf(stderr, "Memory allocation failed for process %d.\n", proc->pcb->pid);
        return;
    }

    // 3. Add the process to the scheduler's ready queue
    add_to_ready_queue(proc);

    // 4. Update the PCB in memory
    update_pcb_in_memory(proc->pcb->pid, proc->pcb);

    // 5. Print memory state for debugging
    print_memory();
}

int main() {
    initialize_memory();
    controller()
    return 0;
}
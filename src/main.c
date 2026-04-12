#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process/pcb.h"
#include "scheduler/scheduler.h"
#include "synchronization/mutex.h"
#include "interpreter/interpreter.h"
#include "interpreter/interpreter.c"
#include "memory/memoryy.h"
#include "memory/memory.c"
#include "process/processs.h"


static int pid = 0;
void controller(){
    initialize_memory();
}

void process_handler(FILE *input_file, int arrival_time) {
    // 1. Read and interpret the input file to create a Process structure
    loadAndInterpret(input_file);

    // 2. Create process
    Process *proc = initProcess(pid++, CountLines(fileContent), arrival_time); // Initialize process with PID and line count
    if (proc == NULL) {
        fprintf(stderr, "Failed to initialize process from file %s.\n", input_file);
        return;
    }

    // 3. Allocate memory for the process
    proc = allocate_memory(proc->pcb->pid, proc);
    if (proc == NULL) {
        fprintf(stderr, "Memory allocation failed for process %d.\n", proc->pcb->pid);
        return;
    }

    // 4. Add the process to the scheduler's ready queue
    add_to_ready_queue(proc);

    // 5. Update the PCB in memory
    update_pcb_in_memory(proc->pcb->pid, proc->pcb);

    // 6. Print memory state for debugging
    print_memory();
};

int main() {
    return 0;
}
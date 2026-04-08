#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include <stdio.h>
#include <math.h>

ReadyQueue mlfq_queues[4];

PCB* execute_mlfq() {
    PCB *current_process = NULL;
    int current_level = -1;

    //Find the highest priority non-empty queue 
    for (int i = 0; i < 4; i++) {
        if (!is_empty(&mlfq_queues[i])) {
            current_process = dequeue(&mlfq_queues[i]);
            current_level = i;
            break;
        }
    }

    if (current_process == NULL) return NULL;

    //  quantum for this leve(2^i)
    int quantum = (int)pow(2, current_level);
    current_process->state = RUNNING;
    
    printf("MLFQ: Selected P%d from Queue %d (Quantum: %d)\n", 
            current_process->pid, current_level, quantum);
    print_all_queues();

    //execute 
    int instructions_run = 0;
    while (instructions_run < quantum) {
        printf("Running P%d | Instruction %d/%d\n", 
                current_process->pid, instructions_run + 1, quantum);
        
        execute_instruction(current_process);
        instructions_run++;

        if (current_process->state == FINISHED){
            printf("Process %d has finished",current_process->pid);
            print_all_queues();
            return current_process; 

        }
        if(current_process->state == BLOCKED) {
            printf("Process %d has finished",current_process->pid);
            print_all_queues();
            return current_process; 
        }
    }

    //the process used its whole quantum, move to the next priority queue
    if (current_process->state == RUNNING) {
        current_process->state = READY;
        int next_level = (current_level < 3) ? current_level + 1 : 3;
        
        enqueue(&mlfq_queues[next_level], current_process);
        printf("P%d used full quantum. Moving to Queue %d.\n", current_process->pid, next_level);
        print_all_queues();
    }

    return current_process;
}
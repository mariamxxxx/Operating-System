#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include "../memory/memoryy.h"
#include <stdio.h>
#include <math.h>

Queue mlfq_queues[4];

void init_mlfq() {
    for (int i = 0; i < 4; i++) {
        init_queue(&mlfq_queues[i]);
    }
}

Process* execute_mlfq() {

    printf("Executing Multi-Level Feedback Queue Algorithm");
    Process *current_process = NULL;
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

    //quantum for this leve(2^i)
    int quantum = (int)pow(2, current_level);
    current_process->pcb->state = RUNNING;
    update_state_in_memory(current_process->pcb->pid, RUNNING);
    
    printf("MLFQ: Selected P%d from Queue %d (Quantum: %d)\n", 
            current_process->pcb->pid, current_level, quantum);

    //execute 
    int instructions_run = 0;
    while (instructions_run < quantum) {
        printf("Running P%d | Instruction %d/%d\n", 
                current_process->pcb->pid, instructions_run + 1, quantum);
        
        execute_instruction(current_process);
        instructions_run++;

        if (current_process->pcb->state == FINISHED){
            printf("Process %d has finished\n",current_process->pcb->pid);
            update_state_in_memory(current_process->pcb->pid, FINISHED);
            print_all_queues();
            return current_process; 

        }
        if(current_process->pcb->state == BLOCKED) {
            printf("Process %d is blocked\n",current_process->pcb->pid);
            update_state_in_memory(current_process->pcb->pid, BLOCKED);
            print_all_queues();
            return current_process; 
        }
    }

    //the process used its whole quantum, move to the next priority queue
    if (current_process->pcb->state == RUNNING) {
        current_process->pcb->state = READY;
        update_state_in_memory(current_process->pcb->pid, READY);
        int next_level = (current_level < 3) ? current_level + 1 : 3;
        
        enqueue(current_process, &mlfq_queues[next_level]);
        printf("P%d used full quantum. Moving to Queue %d.\n", current_process->pcb->pid, next_level);
        print_all_queues();
    }

    return current_process;
}
#include "scheduler.h"
#include "../interpreter/interpreter.h"
#include "../memory/memoryy.h"
#include <stdio.h>
#include <math.h>

Queue mlfq_queues[4];
static Process *current_mlfq_process = NULL;
static int current_mlfq_level = -1;
static int mlfq_ticks_used = 0;

void init_mlfq() {
    for (int i = 0; i < 4; i++) {
        init_queue(&mlfq_queues[i]);
    }

    current_mlfq_process = NULL;
    current_mlfq_level = -1;
    mlfq_ticks_used = 0;
}

Process* execute_mlfq() {
    printf("Executing Multi-Level Feedback Queue Algorithm");
    print_all_queues();

    if (current_mlfq_process == NULL) {
        // Find the highest priority non-empty queue only when the CPU is idle.
        for (int i = 0; i < 4; i++) {
            if (!is_empty(&mlfq_queues[i])) {
                current_mlfq_process = dequeue(&mlfq_queues[i]);
                current_mlfq_level = i;
                mlfq_ticks_used = 0;
                break;
            }
        }
    }

    if (current_mlfq_process == NULL) return NULL;

    //quantum for this leve(2^i)
    int quantum = (int)pow(2, current_mlfq_level);
    printf("MLFQ: Selected P%d from Queue %d (Quantum: %d)\n", 
            current_mlfq_process->pcb->pid, current_mlfq_level, quantum);

    printf("Running P%d | Instruction %d/%d\n", 
            current_mlfq_process->pcb->pid, mlfq_ticks_used + 1, quantum);
    swap_in(current_mlfq_process->pcb->pid);
    sync_pcb_from_memory(current_mlfq_process->pcb->pid, current_mlfq_process->pcb);
    current_mlfq_process->pcb->state = RUNNING;
    update_state_in_memory(current_mlfq_process->pcb->pid, RUNNING);
    print_memory();
    execute_instruction(current_mlfq_process);

    if (current_mlfq_process->pcb->state == FINISHED) {
        printf("Process %d has finished\n", current_mlfq_process->pcb->pid);
        update_state_in_memory(current_mlfq_process->pcb->pid, FINISHED);
        current_mlfq_process = NULL;
        current_mlfq_level = -1;
        mlfq_ticks_used = 0;
        print_all_queues();
        return NULL;
    }

    if (current_mlfq_process->pcb->state == BLOCKED) {
        printf("Process %d is blocked\n", current_mlfq_process->pcb->pid);
        update_state_in_memory(current_mlfq_process->pcb->pid, BLOCKED);
        current_mlfq_process = NULL;
        current_mlfq_level = -1;
        mlfq_ticks_used = 0;
        print_all_queues();
        return NULL;
    }

    mlfq_ticks_used++;

    //the process used its whole quantum, move to the next priority queue
    if (mlfq_ticks_used >= quantum && current_mlfq_process->pcb->state == RUNNING) {
        current_mlfq_process->pcb->state = READY;
        update_state_in_memory(current_mlfq_process->pcb->pid, READY);
        int next_level = (current_mlfq_level < 3) ? current_mlfq_level + 1 : 3;
        
        enqueue(current_mlfq_process, &mlfq_queues[next_level]);
        printf("P%d used full quantum. Moving to Queue %d.\n", current_mlfq_process->pcb->pid, next_level);
        current_mlfq_process = NULL;
        current_mlfq_level = -1;
        mlfq_ticks_used = 0;
        print_all_queues();
    }

    return current_mlfq_process;
}
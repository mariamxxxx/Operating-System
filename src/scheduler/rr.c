#include"scheduler.h"
#include "../interpreter/interpreter.h" // To call the instruction execution
#include "../memory/memoryy.h"
#include <stdio.h>
#include "../process/processs.h"

extern void set_pending_rr_process(Process *process);

static Process *current_rr_process = NULL;
static int rr_ticks_used = 0;


Process* execute_round_robin() {
    printf("Executing Round Robin Algorithm");
    print_all_queues();

    if (current_rr_process == NULL) {
        if (os_ready_queue.head == NULL) {
            printf("Scheduler: no proccesses in the ready queue\n");
            return NULL; // No processes ready to run
        }

        current_rr_process = dequeue(&os_ready_queue);
        rr_ticks_used = 0;
        printf("Scheduler: process %d using is selected for execution using Round Robin Algorithm\n", current_rr_process->pcb->pid);
        print_all_queues();
    }

    int instruction_number = current_rr_process->pcb->pc - (current_rr_process->pcb->memory_bounds[0] + 7) + 1;
    if (instruction_number < 1) {
        instruction_number = rr_ticks_used + 1;
    }
    printf("Running Process %d | Instruction %d (PC: %d)\n", current_rr_process->pcb->pid, instruction_number, current_rr_process->pcb->pc);
    swap_in(current_rr_process->pcb->pid);
    sync_pcb_from_memory(current_rr_process->pcb->pid, current_rr_process->pcb);
    current_rr_process->pcb->state = RUNNING;
    update_state_in_memory(current_rr_process->pcb->pid, RUNNING);
    print_memory();
    execute_instruction(current_rr_process);

    if (current_rr_process->pcb->state == FINISHED) {
        printf("process %d has finished.\n", current_rr_process->pcb->pid);
        update_state_in_memory(current_rr_process->pcb->pid, FINISHED);
        current_rr_process = NULL;
        rr_ticks_used = 0;
        print_all_queues();
        return NULL;
    }

    if (current_rr_process->pcb->state == BLOCKED) {
        printf("Process %d is blocked\n", current_rr_process->pcb->pid);
        update_state_in_memory(current_rr_process->pcb->pid, BLOCKED);
        current_rr_process = NULL;
        rr_ticks_used = 0;
        print_all_queues();
        return NULL;
    }

    rr_ticks_used++;
    if (rr_ticks_used >= time_quantum) {
        current_rr_process->pcb->state = READY;
        update_state_in_memory(current_rr_process->pcb->pid, READY);
        set_pending_rr_process(current_rr_process);
        printf("Process %d time slice ended. Moved to end of Ready Queue.\n", current_rr_process->pcb->pid);
        current_rr_process = NULL;
        rr_ticks_used = 0;
        print_all_queues();
        return NULL;
    }

    return current_rr_process;
}

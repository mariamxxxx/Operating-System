#include"scheduler.h"
#include "../interpreter/interpreter.h" // To call the instruction execution
#include "../memory/memoryy.h"
#include <stdio.h>
#include "../process/processs.h"


Process* execute_round_robin() {
    printf("Executing Round Robin Algorithm");
    print_all_queues();

    if (os_ready_queue.head == NULL) {
        printf("Scheduler: no proccesses in the ready queue\n");
        return NULL; // No processes ready to run
    }
    Process * current_process= dequeue(&os_ready_queue);
    current_process->pcb->state = RUNNING;
    update_state_in_memory(current_process->pcb->pid, RUNNING);
    printf("Scheduler: process %d using is selected for execution using Round Robin Algorithm\n", current_process->pcb->pid);

    print_all_queues();

    for(int i=0; i<time_quantum;i++){
        // The interpreter will use the PC from the PCB to find the instruction and execute it. 

        printf("Running Process %d | Instruction %d (PC: %d)\n", current_process->pcb->pid, i + 1, current_process->pcb->pc);

        execute_instruction(current_process); 
        if (current_process->pcb->state == FINISHED){
            printf("process %d has finished.\n", current_process->pcb->pid);
            update_state_in_memory(current_process->pcb->pid, FINISHED);
            print_all_queues();
            return current_process;
        }    
        if(current_process->pcb->state ==BLOCKED) {
           printf("Process %d is blocked\n",current_process->pcb->pid);
           update_state_in_memory(current_process->pcb->pid, BLOCKED);
           print_all_queues();
           return current_process;
    }
}
    //ba3d ma ykhalas el time slice
    
    current_process->pcb->state = READY; 
    update_state_in_memory(current_process->pcb->pid, READY);
    enqueue(current_process, &os_ready_queue); 
    printf("Process %d time slice ended. Moved to end of Ready Queue.\n", current_process->pcb->pid);
    print_all_queues();

    return current_process;
}

#include"scheduler.h"
#include "../interpreter/interpreter.h" // To call the instruction execution
#include <stdio.h>

PCB* execute_round_robin() {
    PCB *current_process = dequeue(&os_ready_queue);

    if (current_process == NULL) {
        printf("Scheduler: no proccesses in the ready queue\n");
        return NULL; // No processes ready to run
    }
    current_process->state = RUNNING;
    printf("Scheduler: process %d using is selected for execution using Round Robin Algorithm\n", current_process->pid);

    print_all_queues();

    for(int i=0; i<2;i++){
        // The interpreter will use the PC from the PCB to find the instruction and execute it. 

printf("Running Process %d | Instruction %d (PC: %d)\n", current_process->pid, i + 1, current_process->pc);

        execute_instruction(current_process); //check with heba eh esm el instruction
        if (current_process->state == FINISHED){
            printf("process %d has finished.\n", current_process->pid);
            print_all_queues();
            return current_process;
        }    
        if(current_process->state ==BLOCKED) {
           printf("Process %d is blocked\n",current_process->pid);
           print_all_queues();
           return current_process;
    }
}
    //ba3d ma ykhalas el time slice

    current_process->state = READY; 
    enqueue(&os_ready_queue, current_process); 
    printf("Process %d time slice ended. Moved to end of Ready Queue.\n", current_process->pid);
    print_all_queues();
    return current_process;
}

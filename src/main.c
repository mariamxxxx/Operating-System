#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process/pcb.h"
#include "scheduler/scheduler.h"
#include "synchronization/mutex.h"
#include "interpreter/interpreter.h"
#include "memory/memoryy.h"
#include "process/processs.h"

int main() {
    printf("========================================\n");
    printf("  Operating System Simulator Test\n");
    printf("========================================\n\n");

    loadAndInterpret("Program_1.txt");

    // // Test 1: Scheduler Initialization
    // printf("[TEST 1] Initializing Scheduler...\n");
    // init_scheduler();
    // init_mlfq();
    // printf("Scheduler initialized successfully.\n\n");

    // // Test 2: Synchronization Primitives
    // printf("[TEST 2] Initializing Mutex/Semaphores...\n");
    // init_mutex();
    // printf("Mutex initialized successfully.\n\n");

    // // Test 3: Scheduler Operations
    // printf("[TEST 3] Testing Scheduler Operations...\n");
    // SchedulerAlgorithm current_algo = get_current_algo();
    // printf("Current scheduling algorithm: %d\n", current_algo);
    // printf("Scheduler operations completed successfully.\n\n");

    // // Test 4: Process Scheduling
    // printf("[TEST 4] Testing Process Scheduling...\n");
    // PCB* next_process = schedule_next_process(current_algo);
    // if (next_process != NULL) {
    //     printf("Next process scheduled successfully.\n");
    // } else {
    //     printf("No processes available to schedule.\n");
    // }
    // printf("Process scheduling test completed.\n\n");

    // // Test 5: Display Queue Status
    // printf("[TEST 5] Displaying Queue Status...\n");
    // print_all_queues();
    // printf("Queue status displayed.\n\n");

    // printf("========================================\n");
    // printf("  All Tests Completed Successfully!\n");
    // printf("========================================\n");

    return 0;
}

void controller(FILE *input_file){
    
}

int main() {
    init_memory();

    // add file 1
    controller();
    return 0;
}
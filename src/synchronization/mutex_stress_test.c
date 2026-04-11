#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mutex.h"

static PCB make_pcb(int pid, ProcessState state){
    PCB p;
    memset(&p, 0, sizeof(PCB));
    p.pid = pid;
    p.state = state;
    return p;
}

static void test_queue_wraparound_fifo(void){
    Queue q;
    PCB pool[1100];
    int i;

    init_queue(&q);
    for (i = 0; i < 1100; i++) {
        pool[i] = make_pcb(i, READY);
    }

    for (i = 0; i < 900; i++) {
        enqueue(&pool[i], &q);
    }

    for (i = 0; i < 850; i++) {
        assert(dequeue(&q) == &pool[i]);
    }

    for (i = 900; i < 1100; i++) {
        enqueue(&pool[i], &q);
    }

    for (i = 850; i < 900; i++) {
        assert(dequeue(&q) == &pool[i]);
    }

    for (i = 900; i < 1100; i++) {
        assert(dequeue(&q) == &pool[i]);
    }

    assert(is_empty(&q));
}

static void test_many_blocked_waiters_fifo(void){
    PCB owner;
    PCB waiters[20];
    int i;

    owner = make_pcb(1, READY);
    for (i = 0; i < 20; i++) {
        waiters[i] = make_pcb(100 + i, READY);
    }

    init_mutex();
    semWait(&owner, USER_INPUT);

    for (i = 0; i < 20; i++) {
        semWait(&waiters[i], USER_INPUT);
        assert(waiters[i].state == BLOCKED);
    }

    for (i = 0; i < 20; i++) {
        semSignal(USER_INPUT);
        assert(!is_empty(&readyQueue));
        assert(dequeue(&readyQueue) == &waiters[i]);
        assert(waiters[i].state == READY);
        assert(binSem[USER_INPUT] == 0);
    }

    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_INPUT]));

    semSignal(USER_INPUT);
    assert(binSem[USER_INPUT] == 1);
}

static void test_invalid_resource_is_ignored(void){
    PCB p = make_pcb(77, READY);

    init_mutex();
    semWait(&p, (enum RESOURCE)99);
    assert(binSem[USER_INPUT] == 1);
    assert(binSem[USER_OUTPUT] == 1);
    assert(binSem[FILE_RESOURCE] == 1);
    assert(is_empty(&general_blocked_queue));

    semSignal((enum RESOURCE)-1);
    assert(binSem[USER_INPUT] == 1);
    assert(binSem[USER_OUTPUT] == 1);
    assert(binSem[FILE_RESOURCE] == 1);
}

static void test_queue_dequeue_empty_after_churn(void){
    Queue q;
    PCB pool[50];
    int i;

    init_queue(&q);
    for (i = 0; i < 50; i++) {
        pool[i] = make_pcb(200 + i, READY);
    }

    for (i = 0; i < 50; i++) {
        enqueue(&pool[i], &q);
    }

    for (i = 0; i < 50; i++) {
        assert(dequeue(&q) == &pool[i]);
    }

    assert(is_empty(&q));
    assert(dequeue(&q) == NULL);
}

static void test_independent_resource_blocked_queues(void){
    PCB owner_input = make_pcb(301, READY);
    PCB waiter_input = make_pcb(302, READY);
    PCB owner_file = make_pcb(303, READY);
    PCB waiter_file = make_pcb(304, READY);

    init_mutex();

    semWait(&owner_input, USER_INPUT);
    semWait(&waiter_input, USER_INPUT);
    semWait(&owner_file, FILE_RESOURCE);
    semWait(&waiter_file, FILE_RESOURCE);

    assert(waiter_input.state == BLOCKED);
    assert(waiter_file.state == BLOCKED);

    semSignal(USER_INPUT);

    assert(waiter_input.state == READY);
    assert(waiter_file.state == BLOCKED);
    assert(is_empty(&blockedQueues[USER_INPUT]));
    assert(!is_empty(&blockedQueues[FILE_RESOURCE]));

    semSignal(FILE_RESOURCE);
    assert(waiter_file.state == READY);
    assert(is_empty(&blockedQueues[FILE_RESOURCE]));
}

static void assert_queue_state_matches_process_state(const Queue *q, ProcessState expectedState){
    QueueNode *current = q->head;
    while (current != NULL) {
        assert(current->process != NULL);
        assert(current->process->state == expectedState);
        current = current->next;
    }
}

static void assert_mutex_invariants(void){
    int resource;

    for (resource = 0; resource < 3; resource++) {
        assert(binSem[resource] == 0 || binSem[resource] == 1);
        assert_queue_state_matches_process_state(&blockedQueues[resource], BLOCKED);
        assert_queue_state_matches_process_state(&readyQueue, READY);
        if (is_empty(&blockedQueues[resource])) {
            assert(binSem[resource] == 1 || binSem[resource] == 0);
        }
    }

    assert_queue_state_matches_process_state(&general_blocked_queue, BLOCKED);
}

static void test_randomized_fuzz_sequence(void){
    PCB processes[12];
    ProcessState expectedState[12];
    int i;
    int step;
    int resource;
    int processIndex;
    int resourceWasFree;

    srand(12345);
    init_mutex();

    for (i = 0; i < 12; i++) {
        processes[i] = make_pcb(400 + i, READY);
        expectedState[i] = READY;
    }

    for (step = 0; step < 500; step++) {
        resource = rand() % 3;
        processIndex = rand() % 12;

        if ((rand() % 2 == 0) && expectedState[processIndex] == READY) {
            resourceWasFree = binSem[resource];
            semWait(&processes[processIndex], (enum RESOURCE)resource);
            if (resourceWasFree == 1) {
                expectedState[processIndex] = READY;
            } else {
                expectedState[processIndex] = BLOCKED;
            }
        } else {
            semSignal((enum RESOURCE)resource);
            if (!is_empty(&readyQueue)) {
                PCB *released = dequeue(&readyQueue);
                assert(released != NULL);
                for (i = 0; i < 12; i++) {
                    if (&processes[i] == released) {
                        expectedState[i] = READY;
                        break;
                    }
                }
            }
        }

        assert_mutex_invariants();
    }

    init_mutex();
    assert_mutex_invariants();
}

int main(void){
    test_queue_wraparound_fifo();
    test_many_blocked_waiters_fifo();
    test_invalid_resource_is_ignored();
    test_queue_dequeue_empty_after_churn();
    test_independent_resource_blocked_queues();
    test_randomized_fuzz_sequence();

    printf("All mutex stress tests passed.\n");
    return 0;
}

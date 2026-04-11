#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mutex.h"

static PCB make_pcb(int pid, ProcessState state){
    PCB p;
    memset(&p, 0, sizeof(PCB));
    p.pid = pid;
    p.state = state;
    return p;
}

static void test_queue_fifo(void){
    Queue q;
    PCB p1 = make_pcb(1, READY);
    PCB p2 = make_pcb(2, READY);

    init_queue(&q);
    enqueue(&p1, &q);
    enqueue(&p2, &q);

    assert(dequeue(&q) == &p1);
    assert(dequeue(&q) == &p2);
    assert(is_empty(&q));
}

static void test_dequeue_empty_returns_null(void){
    Queue q;

    init_queue(&q);
    assert(dequeue(&q) == NULL);
}

static void test_semwait_acquire_free_resource(void){
    PCB p1 = make_pcb(1, READY);

    init_mutex();
    semWait(&p1, USER_INPUT);

    assert(binSem[USER_INPUT] == 0);
    assert(p1.state == READY);
    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_INPUT]));
}

static void test_semwait_blocks_when_taken(void){
    PCB owner = make_pcb(1, READY);
    PCB waiter = make_pcb(2, READY);

    init_mutex();
    semWait(&owner, USER_OUTPUT);
    semWait(&waiter, USER_OUTPUT);

    assert(waiter.state == BLOCKED);
    assert(!is_empty(&general_blocked_queue));
    assert(!is_empty(&blockedQueues[USER_OUTPUT]));
}

static void test_semwait_same_process_not_enqueued_twice(void){
    PCB owner = make_pcb(1, READY);
    PCB waiter = make_pcb(2, READY);

    init_mutex();
    semWait(&owner, USER_INPUT);
    semWait(&waiter, USER_INPUT);
    semWait(&waiter, USER_INPUT);

    assert(waiter.state == BLOCKED);
    assert(general_blocked_queue.size == 1);
    assert(blockedQueues[USER_INPUT].size == 1);

    semSignal(USER_INPUT);

    assert(waiter.state == READY);
    assert(general_blocked_queue.size == 0);
    assert(blockedQueues[USER_INPUT].size == 0);

    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_INPUT]));
}

static void test_semsignal_unblocks_one_process(void){
    PCB owner = make_pcb(1, READY);
    PCB waiter = make_pcb(2, READY);

    init_mutex();
    semWait(&owner, FILE_RESOURCE);
    semWait(&waiter, FILE_RESOURCE);
    semSignal(FILE_RESOURCE);

    assert(waiter.state == READY);
    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[FILE_RESOURCE]));
    assert(!is_empty(&readyQueue));
    assert(dequeue(&readyQueue) == &waiter);
    assert(binSem[FILE_RESOURCE] == 0);
}

static void test_semsignal_releases_when_no_waiters(void){
    PCB owner = make_pcb(1, READY);

    init_mutex();
    semWait(&owner, USER_INPUT);
    semSignal(USER_INPUT);

    assert(binSem[USER_INPUT] == 1);
    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_INPUT]));
}

static void test_repeated_signal_on_free_resource_is_stable(void){
    init_mutex();

    semSignal(USER_OUTPUT);
    semSignal(USER_OUTPUT);

    assert(binSem[USER_OUTPUT] == 1);
    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_OUTPUT]));
}

static void test_resource_isolation_between_mutexes(void){
    PCB owner_input = make_pcb(10, READY);
    PCB waiter_input = make_pcb(11, READY);

    init_mutex();
    semWait(&owner_input, USER_INPUT);
    semWait(&waiter_input, USER_INPUT);

    semSignal(USER_OUTPUT);

    assert(waiter_input.state == BLOCKED);
    assert(!is_empty(&blockedQueues[USER_INPUT]));
    assert(is_empty(&blockedQueues[USER_OUTPUT]));
    assert(binSem[USER_OUTPUT] == 1);
}

static void test_init_mutex_resets_all_state(void){
    PCB owner = make_pcb(20, READY);
    PCB waiter = make_pcb(21, READY);

    init_mutex();
    semWait(&owner, FILE_RESOURCE);
    semWait(&waiter, FILE_RESOURCE);
    assert(waiter.state == BLOCKED);

    init_mutex();

    assert(binSem[USER_INPUT] == 1);
    assert(binSem[USER_OUTPUT] == 1);
    assert(binSem[FILE_RESOURCE] == 1);
    assert(is_empty(&general_blocked_queue));
    assert(is_empty(&blockedQueues[USER_INPUT]));
    assert(is_empty(&blockedQueues[USER_OUTPUT]));
    assert(is_empty(&blockedQueues[FILE_RESOURCE]));
    assert(is_empty(&readyQueue));
}

int main(void){
    test_queue_fifo();
    test_dequeue_empty_returns_null();
    test_semwait_acquire_free_resource();
    test_semwait_blocks_when_taken();
    test_semwait_same_process_not_enqueued_twice();
    test_semsignal_unblocks_one_process();
    test_semsignal_releases_when_no_waiters();
    test_repeated_signal_on_free_resource_is_stable();
    test_resource_isolation_between_mutexes();
    test_init_mutex_resets_all_state();

    printf("All mutex tests passed.\n");
    return 0;
}

#include <stdio.h>
#include "mutex.h"
#include "../process/pcb.h"
#include "../scheduler/queue.h"
#include "../scheduler/scheduler.h"

// void initQueue(CircularQueue *q){
//     q-> front= -1;
//     q-> rear= -1;
// }

// int isEmpty(CircularQueue *q){
//     return q->front==-1;
// }

// int isFull(CircularQueue* q){
//     return (q->rear +1) %1000 == q-> front;
// }

// void enqueue(CircularQueue* q, int program){
// void enqueue(PCB* process, Queue* q){

//     // if(isFull(q)){//2
//     //     printf("Full Queue\n");
//     //     return;
//     // }
//     if(is_empty(q)){
//         q->front=0;
//         q->rear=0;
//     }
//     else{
//         q-> rear = (q->rear+1) %1000;
//     }
//     q->queue[q->rear]=process;
// }
// PCB* dequeue(CircularQueue* q){
//     if(isEmpty(q)){//2
//         printf("Empty Queue\n");
//         return NULL;
//     }
//     PCB* val = q->queue[q->front];
//     if (q->front == q-> rear){
//         q->front =-1;
//         q->rear =-1;
//     }
//     else{
//         q->front=(q->front +1)%1000;
//     }
//     return val;
// }

int binSem [3]= {1,1,1};
Queue blockedQueues[3];


// Queue blockedQueue;
// Queue readyQueue;


static const char* resource_name(enum RESOURCE r){
    switch (r) {
        case USER_INPUT:
            return "USER_INPUT";
        case USER_OUTPUT:
            return "USER_OUTPUT";
        case FILE_RESOURCE:
            return "FILE_RESOURCE";
        default:
            return "INVALID_RESOURCE";
    }
}

static const char* state_name(ProcessState state){
    switch (state) {
        case READY:
            return "READY";
        case RUNNING:
            return "RUNNING";
        case BLOCKED:
            return "BLOCKED";
        case FINISHED:
            return "FINISHED";
        default:
            return "UNKNOWN_STATE";
    }
}

static int queue_contains(Queue *q, PCB *process){
    QueueNode *current = q->head;

    while (current != NULL) {
        if (current->process == process) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

void init_mutex(void){
    int i;
    for(i = 0; i < 3; i++){
        binSem[i] = 1;
        init_queue(&blockedQueues[i]);
    }
//COMMENT LATER
    init_queue(&general_blocked_queue);
    init_queue(&os_ready_queue);
    //CHECK WITH ROUKA
}

void semWait(PCB* process, enum RESOURCE r){
    if (r < USER_INPUT || r > FILE_RESOURCE) {
        printf("%s is invalid.\n", resource_name(r));
        return;
    }

    if (queue_contains(&general_blocked_queue, process) || queue_contains(&blockedQueues[r], process)) {
        process->state = BLOCKED;
        printf("Process (pid = %d) is %s\n", process->pid, state_name(process->state));
        return;
    }

    if(binSem[r]==1){
        binSem[r]=0;
    printf("%s is now occupied\n", resource_name(r));

    }
    else{
        enqueue(process, &general_blocked_queue);
        printf("Process (pid = %d) is enqueued in the general blocked queue.\n", process -> pid);
        enqueue(process, &blockedQueues[r]);
        process -> state = BLOCKED;
        printf("Process (pid = %d) is enqueued in the %s blocked queue.\n", process->pid, resource_name(r));
    }
}

void semSignal(enum RESOURCE r){
    if (r < USER_INPUT || r > FILE_RESOURCE) {
        printf("%s is invalid.\n", resource_name(r));
        return;
    }

    if (is_empty(&blockedQueues[r])){
        binSem[r]=1;
    printf("%s is now available\n", resource_name(r));

    }
    else{
        PCB* nextProcess=dequeue(&blockedQueues[r]);
        printf("Next Process (pid = %d) is dequeued from the %s blocked queue.\n", nextProcess->pid, resource_name(r));
        int found=0;
        int count=0;
        while (!is_empty(&general_blocked_queue) && !found && count<general_blocked_queue.size){ //to avoid infinite loops
            PCB* val=dequeue(&general_blocked_queue);

            if (val==nextProcess){
                found=1;
                printf("Next Process (pid = %d) is dequeued from the general blocked queue.\n", nextProcess -> pid);
            }
            else
                enqueue(val, &general_blocked_queue);
            count++;
        }
        nextProcess->state = READY;
        printf("Next Process (pid = %d) is %s\n", nextProcess->pid, state_name(nextProcess->state));
        enqueue(nextProcess, &os_ready_queue);
        printf("Next Process (pid = %d) is enqueued in the general ready queue.\n", nextProcess -> pid);
        binSem[r] = 0;
    }
}
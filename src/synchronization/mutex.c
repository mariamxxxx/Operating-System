#include <stdio.h>
#include <pcb.h>
#include "mutex.h"


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
Queue blockedQueue;
Queue blockedQueues[3];
Queue readyQueue;

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
    init_queue(&blockedQueue);
    init_queue(&readyQueue);
    //CHECK WITH ROUKA
}

void semWait(PCB* process, enum RESOURCE r){
    if (r < USER_INPUT || r > FILE_RESOURCE) {
        return;
    }

    if (queue_contains(&blockedQueue, process) || queue_contains(&blockedQueues[r], process)) {
        process->state = BLOCKED;
        return;
    }

    if(binSem[r]==1)
        binSem[r]=0;
    else{
        enqueue(process, &blockedQueue);
        enqueue(process, &blockedQueues[r]);
        process -> state = BLOCKED; 
    }
}

void semSignal(enum RESOURCE r){
    if (r < USER_INPUT || r > FILE_RESOURCE) {
        return;
    }

    if (is_empty(&blockedQueues[r]))
        binSem[r]=1;
    else{
        PCB* nextProcess=dequeue(&blockedQueues[r]);
        
        int found=0;
        int count=0;
        while (!is_empty(&blockedQueue) && !found && count<blockedQueue.size){ //to avoid infinite loops
            PCB* val=dequeue(&blockedQueue);

            if (val==nextProcess)
                found=1;
            else
                enqueue(val, &blockedQueue);
            count++;
        }
        nextProcess->state = READY;
        enqueue(nextProcess, &readyQueue);
        binSem[r] = 0;
    }
}
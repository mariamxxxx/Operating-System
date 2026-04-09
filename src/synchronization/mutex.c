#include <stdio.h>
#include <pcb.h>

enum RESOURCE {
    USER_INPUT,
    USER_OUTPUT,
    FILE_RESOURCE
}; //translate to 0,1,2


typedef struct {
    int queue[1000];
    int front;
    int rear;
} CircularQueue;

void initQueue(CircularQueue *q){
    q-> front= -1;
    q-> rear= -1;
}

int isEmpty(CircularQueue *q){
    return q->front==-1;
}

int isFull(CircularQueue* q){
    return (q->rear +1) %1000 == q-> front;
}

void enqueue(CircularQueue* q, int program){
    if(isFull(q)){//2
        printf("Full Queue\n");
        return;
    }
    if(isEmpty(q)){
        q->front=0;
        q->rear=0;
    }
    else{
        q-> rear = (q->rear+1) %1000;
    }
    q->queue[q->rear]=program;
}
int dequeue(CircularQueue* q){
    if(isEmpty(q)){//2
        printf("Empty Queue\n");
        return -1;
    }
    int val = q->queue[q->front];
    if (q->front == q-> rear){
        q->front =-1;
        q->rear =-1;
    }
    else{
        q->front=(q->front +1)%1000;
    }
    return val;
}

int binSem [3]= {1,1,1};
CircularQueue blockedQueue;
CircularQueue blockedQueues[3];
CircularQueue readyQueue;

void semWait(PCB* program, enum RESOURCE r){
    if(binSem[r]==1)
        binSem[r]=0;
    else{
        enqueue(&blockedQueue, program);
        enqueue(&blockedQueues[r], program);
        program -> state = BLOCKED; 
    }
}

void semSignal(enum RESOURCE r){
    if (isEmpty(&blockedQueues[r]))
        binSem[r]=1;
    else{
        int nextProgram=dequeue(&blockedQueues[r]);
        
        int found=0;
        int count=0;
        while (!isEmpty(&blockedQueue) && !found && count<1000){ //to avoid infinite loops
            int val=dequeue(&blockedQueue);

            if (val==nextProgram)
                found=1;
            else
                enqueue(&blockedQueue,val);
            count++;
        }
        enqueue(&readyQueue, nextProgram);
        binSem[r] = 0;
    }
}
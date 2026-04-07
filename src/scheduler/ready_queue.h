#ifndef READY_QUEUE_H
#define READY_QUEUE_H

#include "../process/pcb.h"

typedef struct QueueNode {
    PCB *process;            // pointer to the actual PCB
    struct QueueNode *next;  // pointer to the next node in line
} QueueNode;

typedef struct {
    QueueNode *head;         // Front of the line (next to be dequeued)
    QueueNode *tail;         // Back of the line (where new processes are enqueued)
    int size;                // Keep track of how many processes are waiting (may be commeneted) hashoof bardo
} ReadyQueue;

//the queue functions we will impelement in ready_queue.c

void init_queue(ReadyQueue *q);
void enqueue(ReadyQueue *q, PCB *process);
PCB* dequeue(ReadyQueue *q);
int is_empty(ReadyQueue *q);

#endif
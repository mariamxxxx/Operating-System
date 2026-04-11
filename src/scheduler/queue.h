#ifndef QUEUE_H
#define QUEUE_H

#include "../process/pcb.h"

typedef struct QueueNode {
    PCB *process;            // pointer to the actual PCB
    struct QueueNode *next;  // pointer to the next node in line
} QueueNode;

typedef struct {
    QueueNode *head;         // Front of the line (next to be dequeued)
    QueueNode *tail;         // Back of the line (where new processes are enqueued)
    int size;                // Keep track of how many processes are waiting (may be commeneted) hashoof bardo
} Queue;

//the queue functions we will impelement in ready_queue.c

void init_queue(Queue *q);
void enqueue(PCB *process, Queue *q);
PCB* dequeue(Queue *q);
int is_empty(Queue *q);
void remove_from_queue(Queue *q, PCB *target);

#endif
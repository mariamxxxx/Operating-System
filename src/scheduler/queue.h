#ifndef QUEUE_H
#define QUEUE_H

#include "../process/pcb.h"
#include "../process/processs.h"


typedef struct QueueNode {
    Process *process;            // pointer to the actual PCB
    struct QueueNode *next;  // pointer to the next node in line
} QueueNode;

typedef struct {
    QueueNode *head;         // Front of the line (next to be dequeued)
    QueueNode *tail;         // Back of the line (where new processes are enqueued)
    int size;                // Keep track of how many processes are waiting (may be commeneted) hashoof bardo
} Queue;

//the queue functions we will impelement in ready_queue.c

void init_queue(Queue *q);
void enqueue(Process *process, Queue *q);
Process *dequeue(Queue *q);
int is_empty(Queue *q);
void remove_from_queue(Queue *q, Process *target);

#endif
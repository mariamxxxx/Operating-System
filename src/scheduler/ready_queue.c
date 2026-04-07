#include "ready_queue.h"
#include <stdlib.h>
#include <stdio.h>

void init_queue(ReadyQueue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void enqueue(ReadyQueue *q, PCB *process) {
    QueueNode *newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->process = process;
    newNode->next = NULL;

    if (q->tail == NULL) {
        q->head = q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail = newNode;
    }
    q->size++;
}

PCB* dequeue(ReadyQueue *q) {
    if (q->head == NULL) return NULL;

    QueueNode *temp = q->head;
    PCB *process = temp->process;
    q->head = q->head->next;

    if (q->head == NULL) q->tail = NULL;

    free(temp);
    q->size--;
    return process;
}
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>



void init_queue(Queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void enqueue(Process *process, Queue *q) {
    if (process == NULL) return;
    
    QueueNode *newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) return;

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

Process* dequeue(Queue *q) {
    if (q->head == NULL) return NULL;

    QueueNode *temp = q->head;
    Process *process = temp->process;
    q->head = q->head->next;

    if (q->head == NULL) q->tail = NULL;

    free(temp);
    q->size--;
    return process;
}

// Support for HRRN (picking non-head processes)
void remove_from_queue(Queue *q, Process *target) {
    if (q->head == NULL || target == NULL) return;

    // If the target is the head
    if (q->head->process == target) {
        dequeue(q);
        return;
    }

    QueueNode *current = q->head;
    while (current->next != NULL && current->next->process != target) {
        current = current->next;
    }

    if (current->next != NULL) {
        QueueNode *temp = current->next;
        current->next = temp->next;
        if (temp == q->tail) {
            q->tail = current;
        }
        free(temp);
        q->size--;
    }
}

int is_empty(Queue *q) {
    return q->head == NULL;
}
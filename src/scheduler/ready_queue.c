#include "ready_queue.h"
#include <stdlib.h>
#include <stdio.h>

void init_queue(ReadyQueue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}
// void init_mlfq() {
//     for (int i = 0; i < 4; i++) {
//         init_queue(&mlfq_queues[i]);
//     }
// }

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


//for hrrn
void remove_from_queue(ReadyQueue *q, PCB *target) {
    if (q->head == NULL || target == NULL) return;

    // if the target is the head
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
// mutex.h
#ifndef MUTEX_H
#define MUTEX_H

#include "../process/processs.h"
#include "../scheduler/queue.h"

enum RESOURCE {
	USER_INPUT,
	USER_OUTPUT,
	FILE_RESOURCE
};

// typedef struct {
// 	PCB* queue[1000];
// 	int front;
// 	int rear;
// } CircularQueue;

extern int binSem[3];
extern Queue blockedQueue;
extern Queue blockedQueues[3];
extern Queue readyQueue;

// void initQueue(CircularQueue* q);
// int isEmpty(CircularQueue* q);
// int isFull(CircularQueue* q);
// void enqueue(PCB* process, CircularQueue* q);
// PCB* dequeue(CircularQueue* q);

void init_mutex(void);
void semWait(Process* process, enum RESOURCE r);
void semSignal(enum RESOURCE r);

#endif

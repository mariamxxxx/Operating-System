#ifndef PROCESS_H
#define PROCESS_H

#include "pcb.h"

extern int next_pid;

// initialize a new process
PCB* create_process(int mem_start, int mem_end, int burst_time);

// Function to free a process when it's finished
void destroy_process(PCB* p);

#endif
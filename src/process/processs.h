#ifndef PROCESS_H
#define PROCESS_H

#include "pcb.h"

extern int next_pid;
#define MAX_STRING 1024
#define MAX_CODE_LINES 30

typedef struct {
    char name[MAX_STRING];
    char value[MAX_STRING];
} ProcessVar;

typedef struct {
    PCB* pcb;
    ProcessVar* var1;
    ProcessVar* var2;
    ProcessVar* var3;
    char code_lines[MAX_CODE_LINES][MAX_STRING];
    int code_line_count;
    int arrival_time;
} Process;

// initialize a new process
PCB* create_process(int mem_start, int mem_end, int burst_time);

// Function to free a process when it's finished
// void destroy_process(PCB* p);

Process* initProcess(int pid, int lines_of_code) ;

#endif
#ifndef PCB_H
#define PCB_H

typedef enum
{
    NEW,   
    READY,
    RUNNING,
    BLOCKED,
    FINISHED,
    SWAPPED,
} ProcessState;


typedef struct{
        int pid;
        ProcessState state;
        int pc;              // Program Counter
        int memory_bounds[2]; // [start, end] indices in memory
    }PCB;




    #endif
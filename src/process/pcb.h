#ifndef PCB_H
#define PCB_H

typedef enum
{
    NEW,   //may be commented hanshoof
    READY,
    RUNNING,
    BLOCKED,
    FINISHED,
    //SWAPPED_OUT
} ProcessState;


typedef struct{
        int pid;
        ProcessState state;
        int pc;              // Program Counter
        int memory_bounds[2]; // [start, end] indices in memory
            
        // for scheduler:
        // int burst_time;      // Needed to calculate HRRN response ratio 
        // int wait_time;       // Needed to calculate HRRN response ratio
    }PCB;




    #endif
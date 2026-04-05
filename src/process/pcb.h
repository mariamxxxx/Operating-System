#ifndef PCB_H
#define PCB_H

typedef enum
{
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    FINISHED,
    SWAPPED_OUT
} ProcessState;

// typedef struct {
//     int pid;
//     ProcessState state;
//     int program_counter;   // saved PC when not running
//     int memory_start;
//     int memory_end;
//     int priority_level;    // for MLFQ (0 = highest)
//     int quantum_used;      // for MLFQ / RR
//     int burst_time;        // total burst time estimate (for HRRN)
//     int waiting_time;      // accumulated waiting time
// } PCB;

#endif
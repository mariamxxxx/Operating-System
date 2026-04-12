#ifndef MEMORYY_H
#define MEMORYY_H

#define MEMORY_SIZE 40
#include "../process/pcb.h"
#include "../process/processs.h"



// for index
typedef struct
{
    int pid;
    int start_index;
    int word_count;
} MapEntry;


typedef enum
{
    VARIABLE,
    CODE_LINE,
    PCB_FIELD
} WordType;

typedef struct
{
    int ownerPid;
    int isFree;
    WordType type; // NEW
    union
    {
        int pid;
        ProcessState state;
        int program_counter;
        int memory_boundary[2];
        ProcessVar var;
        char code_line[MAX_STRING];
    } payload;

} MemoryWord;
// each process: 3 vars, code, pcb elements


// frees all slots, sets owner_pids to 0
void init_memory();

// finds contiguous block for a process and writes its fields into memory
// returns -1 if no space is found
int allocate_memory(int pid, Process *proc);

// clears any spot owned by pid, marks it as free
// called during swap or when process finishes
void free_process_memory(int pid);

// finds matching varname for pid
// returns value if found, else returns NULL
char *read_word(int pid, char *varName);

// Searches the slots owned by pid for a matching key and updates the value.
// If not found, finds the next free slot in that process's range and writes a new entry.
void write_word(int pid, char *key, char *value);

// read a code line stored at a memory index (program counter)
char *read_code_line(int pc);

// writes everyhing owned by pid to disk, marks slots as free
// updates process state to SWAPPED
void swap_out(int pid, int word_count);

// reads everything for pid from disk, marks slots as occupied
void swap_in(int pid);

// prints each mem
// free slots are empty
// owned slotsprint pid, var, value
void print_memory();

#endif
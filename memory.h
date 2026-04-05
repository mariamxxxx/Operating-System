#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_SIZE 40
#define MAX_STRING 256

typedef struct
{
    char varName[MAX_STRING];
    char value[MAX_STRING]; // content of var
    int ownerPid;
    int isFree; // 1 if free, 0 if occupied
} MemoryWord;

// "this exists somehwere else trust me"
extern MemoryWord mem[MEMORY_SIZE];

// frees all slots, sets owner_pids to 0
void init_memory();

// finds contiguous block of num_words,
// marks them as owned by pid
// returns -1 if no space is found
int allocate_memory(int pid, int num_words);

// clears any spot owned by pid, marks it as free
// called during swap or when process finishes
void free_process_memory(int pid);

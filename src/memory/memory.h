#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_SIZE 40
#define MAX_STRING 256

typedef enum
{
    VARIABLE,
    CODE_LINE,
    PCB_FIELD
} WordType;

typedef struct
{
    char varName[MAX_STRING];
    char value[MAX_STRING];
    int ownerPid;
    int isFree;
    WordType type; // NEW
} MemoryWord;
// each process: 3 vars, code, pcb elements

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

// finds matching varname for pid
// returns value if found, else returns NULL
char *read_word(int pid, char *varName);

// Searches the slots owned by pid for a matching key and updates the value.
// If not found, finds the next free slot in that process's range and writes a new entry.
void write_word(int pid, char *key, char *value);

// writes everyhing owned by pid to disk, marks slots as free
// updates process state to SWAPPED
void swap_out(int pid);

// reads everything for pid from disk, marks slots as occupied
void swap_in(int pid);

// prints each mem
// free slots are empty
// owned slotsprint pid, var, value
void print_memory();

#endif
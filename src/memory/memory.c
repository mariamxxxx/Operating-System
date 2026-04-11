#include "memoryy.h"
#include "pcb.h"
#include "processs.h"
#include <stdio.h>
#include <string.h>


#define SWAP_DIR "src/disk"
MemoryWord mem[MEMORY_SIZE];

// HELPERS

// build the file path for the swap file of a given PID.
static void build_swap_path(int pid, char *out, size_t out_size){
    snprintf(out, out_size, "%s/pid_%d.swap", SWAP_DIR, pid);
}

// index for all memory entries
static MapEntry memory_map[MEMORY_SIZE / 2]; 
static int map_i = 0; // tracks the next available index in memory_map

static void build_memory_map(int pid, int start_index, int word_count, int i){
    if (i < MEMORY_SIZE / 2){
        memory_map[i].pid = pid;
        memory_map[i].start_index = start_index;
        memory_map[i].word_count = word_count;
        map_i++;
    }
}

static void update_memory_map(int pid, int new_word_count, int index){
    memory_map[index].word_count = new_word_count;
    memory_map[index].pid = pid;
}

static void delete_memory_map_entry(int pid, int index){
    for (int i = index; i < map_i - 1; i++){
        memory_map[i] = memory_map[i + 1];
    }
    map_i--;
}

// does the actual allocation
static int allocate_block(int pid, int num_words){
    int count = 0;
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (mem[i].isFree){
            count++;
            if (count == num_words){
                int index = i - num_words + 1;
                for (int j = index; j <= i; j++){
                    mem[j].isFree = 0;
                    mem[j].ownerPid = pid;
                }
                return index;
            }
        } else {
            count = 0;
        }
    }

    for (int i = 0; i < map_i; i++){
        if (memory_map[i].word_count >= num_words){
            int index = memory_map[i].start_index;
            swap_out(memory_map[i].pid, memory_map[i].word_count);
            for (int j = index; j < index + num_words; j++){
                mem[j].isFree = 0;
                mem[j].ownerPid = pid;
            }
            update_memory_map(pid, num_words, i);
            return index;
        }
    }

    if (map_i > 0 && memory_map[map_i - 1].start_index + num_words < MEMORY_SIZE){
        int index = memory_map[map_i - 1].start_index;
        swap_out(memory_map[map_i - 1].pid, memory_map[map_i - 1].word_count);
        for (int i = index; i < index + num_words; i++){
            mem[i].isFree = 0;
            mem[i].ownerPid = pid;
        }
        update_memory_map(pid, num_words, map_i - 1);
        return index;
    }

    return -1;
}

// MAIN FUNCTIONS

// initialize memory
void init_memory(){
    for (int i = 0; i < MEMORY_SIZE; i++){
        mem[i].isFree = 1;
        mem[i].ownerPid = -1;
    }
}

// allocate a contiguous block of memory for a process, returning the starting index
int allocate_memory(int pid, Process *proc){
    int code_lines = (sizeof(proc->code_lines) / sizeof(proc->code_lines[0]));
    int num_words = 7 + code_lines; 
    int start_index = allocate_block(pid, num_words);
    if (start_index == -1)
        return -1;
    

    build_memory_map(pid, start_index, num_words, map_i);

    int idx = start_index;
    mem[idx].type = PCB_FIELD;
    mem[idx].payload.pid = proc->pcb.pid;
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.state = proc->pcb.state;
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.program_counter = proc->pcb.pc;
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.memory_boundary[0] = proc->pcb.memory_bounds[0];
    mem[idx].payload.memory_boundary[1] = proc->pcb.memory_bounds[1];
    idx++;

    mem[idx].type = VARIABLE;
    mem[idx].payload.var = proc->var1;
    idx++;

    mem[idx].type = VARIABLE;
    mem[idx].payload.var = proc->var2;
    idx++;

    mem[idx].type = VARIABLE;
    mem[idx].payload.var = proc->var3;
    idx++;

    for (int i = 0; i < code_lines; i++){
        mem[idx].type = CODE_LINE;
        strncpy(mem[idx].payload.code_line, proc->code_lines[i], MAX_STRING - 1);
        mem[idx].payload.code_line[MAX_STRING - 1] = '\0'; //guarantee null termination
        idx++;
    }

    return start_index;
}

void free_process_memory(int pid)
{
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (mem[i].ownerPid == pid){
            mem[i].isFree = 1;
            mem[i].ownerPid = -1;
        }
    }
    for (int i = 0; i < map_i; i++){
        if (memory_map[i].pid == pid){
            delete_memory_map_entry(pid, i);
            break;
        }
    }
}

// read a variable's value from memory for a given PID and variable name. Returns NULL if not found.
char *read_word(int pid, char *varName)
{
    if (varName == NULL)
        return NULL;

    for (int i = 0; i < MEMORY_SIZE; i++){
        if (!mem[i].isFree && mem[i].ownerPid == pid && mem[i].type == VARIABLE && 
            strcmp(mem[i].payload.var.name, varName) == 0){
                return mem[i].payload.var.value;
        }
    }
    return NULL;
}

// write a variable's value to memory for a given PID and variable name. if var already exists, update its value.
// if not, write to the first empty slot within the process block.
// does nothing if no empty slot is available.
void write_word(int pid, char *key, char *value){
    if (key == NULL || value == NULL)
        return;

    // update existing variable if found.
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (!mem[i].isFree && mem[i].ownerPid == pid && mem[i].type == VARIABLE &&
            strcmp(mem[i].payload.var.name, key) == 0){

            strncpy(mem[i].payload.var.value, value, MAX_STRING - 1);
            mem[i].payload.var.value[MAX_STRING - 1] = '\0';
            return;
        }
    }

    // write to the first empty slot within the process block.
    for (int i = 0; i < MEMORY_SIZE; i++){

        if (!mem[i].isFree && mem[i].ownerPid == pid && mem[i].type == VARIABLE &&
            mem[i].payload.var.name[0] == '\0'){

            strncpy(mem[i].payload.var.name, key, MAX_STRING - 1);
            mem[i].payload.var.name[MAX_STRING - 1] = '\0';
            strncpy(mem[i].payload.var.value, value, MAX_STRING - 1);
            mem[i].payload.var.value[MAX_STRING - 1] = '\0';
            return;
        }
    }
}

void swap_out(int pid, int word_count)
{
    if (word_count == 0)
        return;
    
    char path[MAX_STRING];
    build_swap_path(pid, path, sizeof(path));

    FILE *file = fopen(path, "wb");
    if (file == NULL){
        return;
    }

    fwrite(&word_count, sizeof(int), 1, file);
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (!mem[i].isFree && mem[i].ownerPid == pid){
            fwrite(&mem[i], sizeof(MemoryWord), 1, file);
        }
    }

    fclose(file);
    free_process_memory(pid);
}


void swap_in(int pid){
    char path[MAX_STRING];
    build_swap_path(pid, path, sizeof(path));

    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return;
    

    int word_count = 0;
    if (fread(&word_count, sizeof(int), 1, file) != 1 || word_count <= 0){
        fclose(file);
        return;
    }

    int start = allocate_block(pid, word_count);
    if (start == -1){
        fclose(file);
        return;
    }

    for (int i = 0; i < word_count; i++){
        MemoryWord word;
        if (fread(&word, sizeof(MemoryWord), 1, file) != 1)
            break;
        
        mem[start + i] = word;
        mem[start + i].isFree = 0;
        mem[start + i].ownerPid = pid;
    }

    fclose(file);
    remove(path);
}

void print_memory(){
    printf("\n--- MEMORY DUMP ---\n");
    for (int i = 0; i < MEMORY_SIZE; i++){

        if (mem[i].isFree){
            printf("[%d] FREE\n", i);
        } else {

            if (mem[i].type == VARIABLE){
                printf("[%d] PID=%d VAR %s=%s\n",
                       i, mem[i].ownerPid, mem[i].payload.var.name, mem[i].payload.var.value);
            }
            else if (mem[i].type == CODE_LINE){
                printf("[%d] PID=%d CODE %s\n",
                       i, mem[i].ownerPid, mem[i].payload.code_line);
            }
            else{
                printf("[%d] PID=%d PCB\n", i, mem[i].ownerPid);
            }
        }
    }
}

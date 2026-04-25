#include "memoryy.h"
#include "../process/pcb.h"
#include "../process/processs.h"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define SWAP_DIR "src/disk"
MemoryWord mem[MEMORY_SIZE];

// HELPERS
// build the file path for the swap file of a given PID.
static void build_swap_path(int pid, char *out, size_t out_size){
    snprintf(out, out_size, "%s/pid_%d.swap", SWAP_DIR, pid);
}

static void print_swap_file(int pid, const char *action);


// INDEX
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

static void insert_memory_map_entry(int pid, int start_index, int word_count, int index){
    if (map_i >= MEMORY_SIZE / 2 || index < 0 || index > map_i)
        return;

    for (int i = map_i; i > index; i--){
        memory_map[i] = memory_map[i - 1];
    }

    memory_map[index].pid = pid;
    memory_map[index].start_index = start_index;
    memory_map[index].word_count = word_count;
    map_i++;
}

// TO BE CALLED
void update_state_in_memory(int pid, ProcessState state){
    int start = -1;
    for (int i = 0; i < map_i; i++){
        if (memory_map[i].pid == pid){
            start = memory_map[i].start_index;
            break;
        }
    }
    if (start == -1)
        return;

    mem[start + 1].payload.state = state;
}

void update_pc_in_memory(int pid, int pc){
    int start = -1;
    for (int i = 0; i < map_i; i++){
        if (memory_map[i].pid == pid){
            start = memory_map[i].start_index;
            break;
        }
    }
    if (start == -1)
        return;

    mem[start + 2].payload.program_counter = pc;
}

void sync_pcb_from_memory(int pid, PCB *pcb){
    if (pcb == NULL)
        return;

    int start = -1;
    for (int i = 0; i < map_i; i++){
        if (memory_map[i].pid == pid){
            start = memory_map[i].start_index;
            break;
        }
    }

    if (start == -1)
        return;

    pcb->state = mem[start + 1].payload.state;
    pcb->pc = mem[start + 2].payload.program_counter;
    pcb->memory_bounds[0] = mem[start + 3].payload.memory_boundary[0];
    pcb->memory_bounds[1] = mem[start + 3].payload.memory_boundary[1];
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
    // can add delete here but more redundant
}

char *read_code_line(int pc){
    printf("read_code_line: pc=%d\n", pc);
    if (pc < 0 || pc >= MEMORY_SIZE){
        printf("read_code_line: pc out of bounds\n");
        return NULL;
    }

    printf("read_code_line: mem[%d].isFree=%d, type=%d\n", pc, mem[pc].isFree, mem[pc].type);
    if (mem[pc].isFree || mem[pc].type != CODE_LINE){
        printf("read_code_line: not a valid code line\n");
        return NULL;
    }

    printf("read_code_line: returning '%s'\n", mem[pc].payload.code_line);
    return mem[pc].payload.code_line;
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
                build_memory_map(pid, index, num_words, map_i);
                return index;
            }
        }
        else{
            count = 0;
        }
    }
    // check first process with num_words >=
    for (int i = 0; i < map_i; i++){
        if (memory_map[i].word_count >= num_words){
            int index = memory_map[i].start_index;
            int insert_at = i;
            swap_out(memory_map[i].pid, memory_map[i].word_count);
            for (int j = index; j < index + num_words; j++){
                mem[j].isFree = 0;
                mem[j].ownerPid = pid;
            }
            insert_memory_map_entry(pid, index, num_words, insert_at);
            return index;
        }
    }
    // edge case: if last process + remaining empty space in memory can fit process
    // condition checks en el process mesh atwal mn el memory
    if (map_i > 0){
        int last = map_i - 1;
        int end_of_last = memory_map[last].start_index + memory_map[last].word_count;
        int available = memory_map[last].word_count + (MEMORY_SIZE - end_of_last);
        if (available >= num_words){
            int index = memory_map[last].start_index;
            int insert_at = last;
            swap_out(memory_map[last].pid, memory_map[last].word_count);
            for (int i = index; i < index + num_words; i++){
                mem[i].isFree = 0;
                mem[i].ownerPid = pid;
            }
            insert_memory_map_entry(pid, index, num_words, insert_at);
            return index;
        }
    }
    return -1;
}

// MAIN FUNCTIONS
// initialize memory
void init_memory()
{
    for (int i = 0; i < MEMORY_SIZE; i++){
        mem[i].isFree = 1;
        mem[i].ownerPid = -1;
        memset(&mem[i].payload, 0, sizeof(mem[i].payload));
    }
}

// allocate a contiguous block of memory for a process, returning the starting index
Process *allocate_memory(int pid, Process *proc){
    printf("allocate_memory: PID=%d, code_line_count=%d\n", pid, proc->code_line_count);
    if (proc->pcb == NULL || proc->code_line_count > MAX_CODE_LINES)
        return NULL;

    int code_lines = proc->code_line_count;
    int num_words = 7 + code_lines; // 4 pcb fields + 3 vars + code lines
    printf("allocate_memory: num_words=%d\n", num_words);
    int start_index = allocate_block(pid, num_words);
    printf("allocate_memory: start_index=%d\n", start_index);
    if (start_index == -1)
        return NULL;

    int idx = start_index;
    mem[idx].type = PCB_FIELD;
    mem[idx].payload.pid = proc->pcb->pid;
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.state = proc->pcb->state;
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.program_counter = start_index + 7; // point to first code line
    idx++;

    mem[idx].type = PCB_FIELD;
    mem[idx].payload.memory_boundary[0] = start_index;
    mem[idx].payload.memory_boundary[1] = start_index + num_words - 1; // point to last code line
    idx++;

    mem[idx].type = VARIABLE;
    if (proc->var1 != NULL)
        mem[idx].payload.var = *proc->var1;
    else
        memset(&mem[idx].payload.var, 0, sizeof(mem[idx].payload.var));
    idx++;

    mem[idx].type = VARIABLE;
    if (proc->var2 != NULL) 
        mem[idx].payload.var = *proc->var2;
    else 
        memset(&mem[idx].payload.var, 0, sizeof(mem[idx].payload.var));
    
    idx++;

    mem[idx].type = VARIABLE;
    if (proc->var3 != NULL){
        mem[idx].payload.var = *proc->var3;
    } else {
        memset(&mem[idx].payload.var, 0, sizeof(mem[idx].payload.var));
    }
    idx++;

    for (int i = 0; i < code_lines; i++){
        printf("allocate_memory: setting mem[%d] to CODE_LINE, code='%s'\n", idx, proc->code_lines[i]);
        mem[idx].type = CODE_LINE;
        strncpy(mem[idx].payload.code_line, proc->code_lines[i], MAX_STRING - 1);
        mem[idx].payload.code_line[MAX_STRING - 1] = '\0'; // guarantee null termination
        idx++;
    }

    // Keep PCB runtime fields in sync with allocated memory layout.
    proc->pcb->pc = start_index + 7;
    printf("allocate_memory: set PC to %d\n", proc->pcb->pc);
    proc->pcb->memory_bounds[0] = start_index;
    proc->pcb->memory_bounds[1] = start_index + num_words - 1;
    printf("allocate_memory: memory_bounds [%d, %d]\n", proc->pcb->memory_bounds[0], proc->pcb->memory_bounds[1]);

    return proc;
}

// read a variable's value from memory for a given PID and variable name. Returns NULL if not found.
char *read_word(int pid, char *varName){
    if (varName == NULL)
        return NULL;

    for (int i = 0; i < MEMORY_SIZE; i++){
        if (!mem[i].isFree && mem[i].ownerPid == pid && mem[i].type == VARIABLE &&
            strcmp(mem[i].payload.var.name, varName) == 0)
            return mem[i].payload.var.value;
        
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
            mem[i].payload.var.name[0] == '\0')
        {

            strncpy(mem[i].payload.var.name, key, MAX_STRING - 1);
            mem[i].payload.var.name[MAX_STRING - 1] = '\0';
            strncpy(mem[i].payload.var.value, value, MAX_STRING - 1);
            mem[i].payload.var.value[MAX_STRING - 1] = '\0';
            return;
        }
    }
}

void swap_out(int pid, int word_count){
    if (word_count == 0)
        return;

    char path[MAX_STRING];
    build_swap_path(pid, path, sizeof(path));

    FILE *file = fopen(path, "wb");
    if (file == NULL)
        return;
    

    fwrite(&word_count, sizeof(int), 1, file);
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (!mem[i].isFree && mem[i].ownerPid == pid)
            fwrite(&mem[i], sizeof(MemoryWord), 1, file);
        
    }

    fclose(file);
    print_swap_file(pid, "OUT");
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
    print_swap_file(pid, "IN");
    remove(path);

    // update stale addresses
    int old_start = mem[start + 3].payload.memory_boundary[0];
    int offset = start - old_start;

    mem[start + 2].payload.program_counter += offset;
    mem[start + 3].payload.memory_boundary[0] = start;
    mem[start + 3].payload.memory_boundary[1] += offset;

}

void print_memory(){
    printf("\n--- MEMORY DUMP ---\n");
    for (int i = 0; i < MEMORY_SIZE; i++){
        if (mem[i].isFree)
            printf("[%d] FREE\n", i);
        else
        {
            if (mem[i].type == VARIABLE){
                printf("[%d] PID=%d VAR %s=%s\n",
                       i, mem[i].ownerPid, mem[i].payload.var.name, mem[i].payload.var.value);
            } else if (mem[i].type == CODE_LINE){
                printf("[%d] PID=%d CODE %s\n",
                       i, mem[i].ownerPid, mem[i].payload.code_line);
            } else {
                printf("[%d] PID=%d pid=%d\n", i, mem[i].ownerPid, mem[i].ownerPid);
                printf("[%d] PID=%d state=%d\n", i+1, mem[i].ownerPid, mem[i].payload.state);
                printf("[%d] PID=%d pc=%d\n", i+2, mem[i].ownerPid, mem[i].payload.program_counter);
                printf("[%d] PID=%d bounds=[%d, %d]\n", i+3, mem[i].ownerPid,
                       mem[i+3].payload.memory_boundary[0], mem[i+3].payload.memory_boundary[1]);
                i = i + 3; // skip the next 3 entries which are part of the same PCB
            }
        }
    }
}

static void print_swap_file(int pid, const char *action){
    char path[MAX_STRING];
    build_swap_path(pid, path, sizeof(path));

    FILE *file = fopen(path, "rb");
    if (file == NULL){
        printf("No swap file found for PID %d\n", pid);
        return;
    }

    int word_count = 0;
    if (fread(&word_count, sizeof(int), 1, file) != 1 || word_count <= 0){
        printf("Swap file for PID %d is empty or corrupt\n", pid);
        fclose(file);
        return;
    }

    printf("\n--- SWAP %s FILE: PID %d (%d words) ---\n", action, pid, word_count);
    for (int i = 0; i < word_count; i++){
        MemoryWord word;
        if (fread(&word, sizeof(MemoryWord), 1, file) != 1)
            break;

        switch (word.type){
        case PCB_FIELD:
            printf("[%d] PCB ", i);
            if (i == 0)
                printf("pid=%d\n", word.payload.pid);
            else if (i == 1)
                printf("state=%d\n", word.payload.state);
            else if (i == 2)
                printf("pc=%d\n", word.payload.program_counter);
            else if (i == 3)
                printf("bounds=[%d, %d]\n", word.payload.memory_boundary[0], word.payload.memory_boundary[1]);
            else
                printf("\n");
            break;
        case VARIABLE:
            printf("[%d] VAR %s=%s\n", i, word.payload.var.name, word.payload.var.value);
            break;
        case CODE_LINE:
            printf("[%d] CODE %s\n", i, word.payload.code_line);
            break;
        default:
            printf("[%d] UNKNOWN type=%d\n", i, word.type);
        }
    }

    fclose(file);
}

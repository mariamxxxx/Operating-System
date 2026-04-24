
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "../memory/memoryy.h"
// #include "../os/syscalls.h"
// #include "../scheduler/scheduler.h"
// #include "../scheduler/queue.h"
// #include "../process/processs.h"

// int CountLines(char* rawData) {
//     int countLines = 0;
//     for (char* p = rawData; *p; p++) {
//         if (*p == '\n') countLines++;
//     }
//     return countLines;
// }


// char* AvailableFunctions [] = {
//     "semWait", "semSignal", "assign", "print", "printFromTo","writeFile","readFile"
// };
// char* AvailableFunctionsOpcodes[] = {
//     "000", "001", "010", "011", "100", "101", "110"
// };

// static char** SplitInstruction(char* line, int *count) {
//     int capacity = 4;
//     char **tokens = (char **) malloc(capacity * sizeof(char *));
//     *count = 0;

//     char* saveptr;
//     char *token = strtok_r(line, " \t\r", &saveptr);
//     while (token != NULL) {
//         if (*count >= capacity) {
//             capacity *= 2;
//             tokens = (char **) realloc(tokens, capacity * sizeof(char *));
//         }
//         tokens[*count] = token;
//         (*count)++;
//         token = strtok_r(NULL, " \t\r", &saveptr);
//     }

//     return tokens;
// }



// void parseInstructionsIntoMemory(char* rawData , Process* process) {
//     printf("\n=================================================\n");
//     printf("[LOAD] Loading program into memory | PID = %d\n", process->pcb->pid);
//     printf("=================================================\n");

//     char* saveptr;
//     char* line = strtok_r(rawData, "\n", &saveptr); // Get first line

//     int i = 0; 
    
//     while (line != NULL) {
//         printf("\n[PARSE] Instruction line %d\n", i);

//         char* instruction = parseLineToOpcode(line); 

//         if (instruction != NULL && i < MAX_CODE_LINES) {
//             strcpy(process->code_lines[i], instruction);
//             printf("[MEMORY] Stored instruction at code_lines[%d]\n", i);

//             // free(instruction);
//             i++;
//             process->code_line_count++;
//         } else {
//             printf("[INFO] Instruction skipped (empty or memory limit reached).\n");
//         }

//         line = strtok_r(NULL, "\n", &saveptr); // Get next line
//         printf("parseInstructionsIntoMemory: next line: '%s'\n", line ? line : "NULL");
//     }

//     printf("\n[MEMORY] Total instructions loaded: %d\n",process->code_line_count);
//     allocate_memory(process->pcb->pid, process);
//     printf("[MEMORY] Memory allocated for PID %d\n", process->pcb->pid);
//     add_process_to_scheduler(process);

//     printf("[SCHEDULER] Process PID %d added to scheduler\n",process->pcb->pid);

//     printf("=================================================\n");
//     printf("[LOAD] Program loading complete for PID %d\n", process->pcb->pid);
//     printf("=================================================\n");

//     free(rawData); 
// }

// extern void loadAndInterpret(char* filename, int arrival_time) { 
//     printf("\n=================================================\n");
//     printf("[LOAD] Loading program file: %s | Arrival time: %d\n",
//            filename, arrival_time);
//     printf("=================================================\n");

//     if (filename == NULL) {
//         printf("[ERROR] Filename is NULL. Aborting load.\n");
//         return;
//     }
    
//     char* fileContent = readFile(filename);
       
//     if (fileContent == NULL) {
//         printf("[ERROR] Failed to read file: %s\n", filename);
//         return;
//     }

//     Process *process = initProcess(arrival_time);
//     if (process == NULL) {
//         printf("[ERROR] Process initialization failed.\n");
//         free(fileContent);
//         return;
//     }

//     printf("[INFO] Process created successfully | PID = %d\n",
//            process->pcb->pid);

//     char *count_copy = strdup(fileContent);
//     if (count_copy != NULL) {
//         int lines = CountLines(count_copy);
//         printf("[INFO] Program contains %d lines\n",
//                lines);
//         free(count_copy);
//     }

//     parseInstructionsIntoMemory(fileContent, process);
//     printf("[LOAD] Program %s fully loaded and ready for execution\n",
//            filename);
//     printf("=================================================\n");
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
static char* compat_strtok_r(char* str, const char* delim, char** saveptr) {
    char* token_start;
    char* p;

    if (str != NULL) {
        p = str;
    } else if (saveptr != NULL && *saveptr != NULL) {
        p = *saveptr;
    } else {
        return NULL;
    }

    p += strspn(p, delim);
    if (*p == '\0') {
        if (saveptr != NULL) {
            *saveptr = p;
        }
        return NULL;
    }

    token_start = p;
    p = token_start + strcspn(token_start, delim);

    if (*p == '\0') {
        if (saveptr != NULL) {
            *saveptr = p;
        }
    } else {
        *p = '\0';
        if (saveptr != NULL) {
            *saveptr = p + 1;
        }
    }

    return token_start;
}

#define strtok_r(str, delim, saveptr) compat_strtok_r((str), (delim), (saveptr))
#endif
#include <stdarg.h>

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/queue.h"
#include "../process/processs.h"

#ifdef GUI_MODE
// Link to the GUI logger
extern void gui_log(const char *format, ...);
#define LOGF(...) gui_log(__VA_ARGS__)
#else
#define LOGF(...) printf(__VA_ARGS__)
#endif

int CountLines(char* rawData) {
    int countLines = 0;
    for (char* p = rawData; *p; p++) {
        if (*p == '\n') countLines++;
    }
    return countLines;
}


char* AvailableFunctions [] = {
    "semWait", "semSignal", "assign", "print", "printFromTo","writeFile","readFile"
};
char* AvailableFunctionsOpcodes[] = {
    "000", "001", "010", "011", "100", "101", "110"
};

static char** SplitInstruction(char* line, int *count) {
    int capacity = 4;
    char **tokens = (char **) malloc(capacity * sizeof(char *));
    *count = 0;

    char* saveptr;
    char *token = strtok_r(line, " \t\r", &saveptr);
    while (token != NULL) {
        if (*count >= capacity) {
            capacity *= 2;
            tokens = (char **) realloc(tokens, capacity * sizeof(char *));
        }
        tokens[*count] = token;
        (*count)++;
        token = strtok_r(NULL, " \t\r", &saveptr);
    }

    return tokens;
}

// static char* parseLineToOpcode(char* line) {
//     printf("parseLineToOpcode: parsing line: '%s'\n", line);
//     int token_count = 0;
//     char *line_copy = strdup(line);
//     if (line_copy == NULL) {
//         printf("parseLineToOpcode: strdup failed\n");
//         return NULL;
//     }

//     char **instruction = SplitInstruction(line_copy, &token_count);
//     printf("parseLineToOpcode: token_count=%d\n", token_count);
//     if (token_count == 0) {
//         printf("parseLineToOpcode: no tokens, returning NULL\n");
//         free(instruction);
//         free(line_copy);
//         return NULL;
//     }

//     const char *instruction_name = instruction[0];
//     printf("parseLineToOpcode: instruction_name='%s'\n", instruction_name);
//     const char *opcode = instruction_name;
//     char result[256] = {0};

//     for (int i = 0; i < 7; i++) {
//         if (strcmp(instruction_name, AvailableFunctions[i]) == 0) {
//             opcode = AvailableFunctionsOpcodes[i];
//             printf("parseLineToOpcode: matched opcode='%s'\n", opcode);
//             break;
//         }
//     }

//     printf("parseLineToOpcode: final opcode='%s'\n", opcode);
//     strcat(result, opcode);

//     for (int token_index = 1; token_index < token_count; token_index++) {
//         char *token = instruction[token_index];
//         const char *mapped = token;

//         for (int j = 0; j < 7; j++) {
//             if (strcmp(token, AvailableFunctions[j]) == 0) {
//                 mapped = AvailableFunctionsOpcodes[j];
//                 printf("parseLineToOpcode: converted token '%s' to opcode '%s'\n", token, mapped);
//                 break;
//             }
//         }

//         strcat(result, "*");
//         strcat(result, mapped);
//     }

//     printf("parseLineToOpcode: result='%s'\n", result);
//     free(instruction);
//     free(line_copy);
//     return strdup(result);
// }

static char* parseLineToOpcode(char* line) {
    LOGF("[TRANSLATE] Parsing line: \"%s\"\n", line);
    int token_count = 0;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        LOGF("[ERROR] Memory allocation failed during instruction parsing.\n");
        return NULL;
    }

    char **instruction = SplitInstruction(line_copy, &token_count);
    if (token_count == 0) {
        free(instruction);
        free(line_copy);
        return NULL;
    }

    const char *instruction_name = instruction[0];
    const char *opcode = instruction_name;
    char result[256] = {0};

    for (int i = 0; i < 7; i++) {
        if (strcmp(instruction_name, AvailableFunctions[i]) == 0) {
            opcode = AvailableFunctionsOpcodes[i];
            break;
        }
    }

    strcat(result, opcode);

    for (int token_index = 1; token_index < token_count; token_index++) {
        char *token = instruction[token_index];
        const char *mapped = token;

        for (int j = 0; j < 7; j++) {
            if (strcmp(token, AvailableFunctions[j]) == 0) {
                mapped = AvailableFunctionsOpcodes[j];
                break;
            }
        }

        strcat(result, "*");
        strcat(result, mapped);
    }

    LOGF("[TRANSLATE] Opcode generated: \"%s\"\n", result);

    free(instruction);
    free(line_copy);
    return strdup(result);
}

void parseInstructionsIntoMemory(char* rawData , Process* process) {
    LOGF("=================================================\n");
    LOGF("[LOAD] Loading program into memory | PID = %d\n", process->pcb->pid);
    LOGF("=================================================\n");

    char* saveptr;
    char* line = strtok_r(rawData, "\n", &saveptr); // Get first line

    int i = 0; 
    
    while (line != NULL) {
        LOGF("[PARSE] Instruction line %d\n", i);

        char* instruction = parseLineToOpcode(line); 

        if (instruction != NULL && i < MAX_CODE_LINES) {
            strcpy(process->code_lines[i], instruction);
            LOGF("[MEMORY] Stored instruction at code_lines[%d]\n", i);

            // free(instruction);
            i++;
            process->code_line_count++;
        } else {
            LOGF("[INFO] Instruction skipped (empty or memory limit reached).\n");
        }

        line = strtok_r(NULL, "\n", &saveptr); // Get next line
        LOGF("parseInstructionsIntoMemory: next line: '%s'\n", line ? line : "NULL");
    }

    LOGF("[MEMORY] Total instructions loaded: %d\n",process->code_line_count);
    allocate_memory(process->pcb->pid, process);
    LOGF("[MEMORY] Memory allocated for PID %d\n", process->pcb->pid);
    add_process_to_scheduler(process);

    LOGF("[SCHEDULER] Process PID %d added to scheduler\n",process->pcb->pid);

    LOGF("=================================================\n");
    LOGF("[LOAD] Program loading complete for PID %d\n", process->pcb->pid);
    LOGF("=================================================\n");

    free(rawData); 
}

extern void loadAndInterpret(char* filename, int arrival_time) { 
    LOGF("=================================================\n");
    LOGF("[LOAD] Loading program file: %s | Arrival time: %d\n",
        filename, arrival_time);
    LOGF("=================================================\n");

    if (filename == NULL) {
        LOGF("[ERROR] Filename is NULL. Aborting load.\n");
        return;
    }
    
    char* fileContent = readFile(filename);
        
    if (fileContent == NULL) {
        LOGF("[ERROR] Failed to read file: %s\n", filename);
        return;
    }

    Process *process = initProcess(arrival_time);
    if (process == NULL) {
        LOGF("[ERROR] Process initialization failed.\n");
        free(fileContent);
        return;
    }

    LOGF("[INFO] Process created successfully | PID = %d\n",
           process->pcb->pid);

    char *count_copy = strdup(fileContent);
    if (count_copy != NULL) {
        int lines = CountLines(count_copy);
         LOGF("[INFO] Program contains %d lines\n",
               lines);
        free(count_copy);
    }

    parseInstructionsIntoMemory(fileContent, process);
    LOGF("[LOAD] Program %s fully loaded and ready for execution\n",
           filename);
    LOGF("=================================================\n");
}
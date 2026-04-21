
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

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/queue.h"
#include "../process/processs.h"

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
    printf("\n[TRANSLATE] Parsing line: \"%s\"\n", line);
    int token_count = 0;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        printf("[ERROR] Memory allocation failed during instruction parsing.\n");
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

    printf("[TRANSLATE] Opcode generated: \"%s\"\n", result);

    free(instruction);
    free(line_copy);
    return strdup(result);
}

void parseInstructionsIntoMemory(char* rawData , Process* process) {
    printf("parseInstructionsIntoMemory: loading program for PID %d\n", process->pcb->pid);
    printf("parseInstructionsIntoMemory: rawData length=%zu\n", strlen(rawData));

    char* saveptr;
    char* line = strtok_r(rawData, "\n", &saveptr); // Get first line

    int i = 0; 
    
    while (line != NULL) {
        printf("parseInstructionsIntoMemory: processing line %d: '%s'\n", i, line);

        char* instruction = parseLineToOpcode(line); 
        printf("parseInstructionsIntoMemory: parsed instruction: '%s'\n", instruction ? instruction : "NULL");

        if (instruction != NULL && i < MAX_CODE_LINES) {
            strcpy(process->code_lines[i], instruction);
            printf("parseInstructionsIntoMemory: stored at index %d\n", i);

            // free(instruction);
            i++;
            process->code_line_count++;
        } else {
            printf("parseInstructionsIntoMemory: skipped line %d\n", i);
        }
        // printf("parseInstructionsIntoMemory: %d, %s", i, instruction);

        line = strtok_r(NULL, "\n", &saveptr); // Get next line
        printf("parseInstructionsIntoMemory: next line: '%s'\n", line ? line : "NULL");
    }

    printf("parseInstructionsIntoMemory: total code_line_count=%d\n", process->code_line_count);
    allocate_memory(process->pcb->pid, process);
    printf("parseInstructionsIntoMemory: after allocate_memory, PC=%d\n", process->pcb->pc);
    add_process_to_scheduler(process);

    free(rawData); 
}

extern void loadAndInterpret(char* filename, int arrival_time) { 
    printf("loadAndInterpret: filename='%s', arrival_time=%d\n", filename, arrival_time);
    if (filename == NULL) {
        printf("[ERROR] Filename is NULL. Cannot proceed.\n");
        return;
    }
    
    char* fileContent = readFile(filename);
    printf("loadAndInterpret: fileContent length=%zu\n", fileContent ? strlen(fileContent) : 0);
       
    if (fileContent == NULL) {
        printf("[ERROR] Could not load %s - readFile returned NULL\n", filename);
        printf("[ERROR] File may not exist or read permission denied.\n");
        return;
    }

    Process *process = initProcess(arrival_time);
    printf("loadAndInterpret: created process PID=%d\n", process ? process->pcb->pid : -1);
    if (process == NULL) {
        free(fileContent);
        return;
    }

    char *count_copy = strdup(fileContent);
    if (count_copy != NULL) {
        int lines = CountLines(count_copy);
        printf("loadAndInterpret: CountLines returned %d\n", lines);
        free(count_copy);
    }

    parseInstructionsIntoMemory(fileContent, process);
    printf("loadAndInterpret: done\n");
}
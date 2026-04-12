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
    char* line = strtok(rawData, "\n");
    while (line != NULL) {
        countLines++;
        line = strtok(NULL, "\n");
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

    char *token = strtok(line, " \t\r");
    while (token != NULL) {
        if (*count >= capacity) {
            capacity *= 2;
            tokens = (char **) realloc(tokens, capacity * sizeof(char *));
        }
        tokens[*count] = token;
        (*count)++;
        token = strtok(NULL, " \t\r");
    }

    return tokens;
}

static char* parseLineToOpcode(char* line) {
    int token_count = 0;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
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

    char *instructionParameter1 = (token_count > 1) ? instruction[1] : NULL;

    if (strcmp(opcode, "000") == 0 || strcmp(opcode, "001") == 0) {
        strcat(result, "*");
        if (instructionParameter1 != NULL && strcmp(instructionParameter1, "userInput") == 0) {
            strcat(result, "0");
        }
        else if (instructionParameter1 != NULL && strcmp(instructionParameter1, "userOutput") == 0) {
            strcat(result, "1");
        }
        else if (instructionParameter1 != NULL && strcmp(instructionParameter1, "file") == 0) {
            strcat(result, "2");
        }
        else if (instructionParameter1 != NULL) {
            strcat(result, instructionParameter1);
        }

    }
    else {
        if (instructionParameter1 != NULL) {
            strcat(result, "*");
            strcat(result, instructionParameter1);
        }

        if (token_count > 2 && instruction[2] != NULL) {
            strcat(result, "*");
            strcat(result, instruction[2]);
        }

        if (token_count > 3 && instruction[3] != NULL) {
            strcat(result, "*");
            strcat(result, instruction[3]);
        }
    }

    free(instruction);
    free(line_copy);
    return strdup(result);
}

void parseInstructionsIntoMemory(char* rawData , Process* process) {
    printf("parseInstructionsIntoMemory: loading program\n");

    char* line = strtok(rawData, "\n"); // Get first line

    int i = 0; 
    
    while (line != NULL) {

        char* instruction = parseLineToOpcode(line); 

        if (instruction != NULL && i < MAX_CODE_LINES) {
            strcpy(process->code_lines[i], instruction);

            // free(instruction);
            i++;
            process->code_line_count++;
        }
        // printf("parseInstructionsIntoMemory: %d, %s", i, instruction);

        line = strtok(NULL, "\n"); // Get next line
    }

    allocate_memory(process->pcb->pid, process);
    add_process_to_scheduler(process);

    free(rawData); 
}

extern void loadAndInterpret(char* filename, int arrival_time) { 
    if (filename == NULL) {
        printf("[ERROR] Filename is NULL. Cannot proceed.\n");
        return;
    }
    
    char* fileContent = readFile(filename);
       
    if (fileContent == NULL) {
        printf("[ERROR] Could not load %s - readFile returned NULL\n", filename);
        printf("[ERROR] File may not exist or read permission denied.\n");
        return;
    }

    Process *process = initProcess(arrival_time);
    if (process == NULL) {
        free(fileContent);
        return;
    }

    char *count_copy = strdup(fileContent);
    if (count_copy != NULL) {
        (void) CountLines(count_copy);
        free(count_copy);
    }

    parseInstructionsIntoMemory(fileContent, process);
}
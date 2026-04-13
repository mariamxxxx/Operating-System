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

static char* parseLineToOpcode(char* line) {
    printf("parseLineToOpcode: parsing line: '%s'\n", line);
    int token_count = 0;
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        printf("parseLineToOpcode: strdup failed\n");
        return NULL;
    }

    char **instruction = SplitInstruction(line_copy, &token_count);
    printf("parseLineToOpcode: token_count=%d\n", token_count);
    if (token_count == 0) {
        printf("parseLineToOpcode: no tokens, returning NULL\n");
        free(instruction);
        free(line_copy);
        return NULL;
    }

    const char *instruction_name = instruction[0];
    printf("parseLineToOpcode: instruction_name='%s'\n", instruction_name);
    const char *opcode = instruction_name;
    char result[256] = {0};

    for (int i = 0; i < 7; i++) {
        if (strcmp(instruction_name, AvailableFunctions[i]) == 0) {
            opcode = AvailableFunctionsOpcodes[i];
            printf("parseLineToOpcode: matched opcode='%s'\n", opcode);
            break;
        }
    }

    printf("parseLineToOpcode: final opcode='%s'\n", opcode);
    strcat(result, opcode);

    char *instructionParameter1 = (token_count > 1) ? instruction[1] : NULL;
    printf("parseLineToOpcode: param1='%s'\n", instructionParameter1 ? instructionParameter1 : "NULL");

    if (strcmp(opcode, "000") == 0 || strcmp(opcode, "001") == 0) {
        printf("parseLineToOpcode: semWait/semSignal handling\n");
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
        printf("parseLineToOpcode: other instruction handling\n");
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

    printf("parseLineToOpcode: result='%s'\n", result);
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
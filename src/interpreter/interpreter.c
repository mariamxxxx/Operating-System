#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../synchronization/mutex.h"
#include "parser.h"

char* substring(const char* src, int start, int length) {
    char* sub = malloc(length + 1);
    if (sub == NULL) {
        return NULL;
    }

    strncpy(sub, src + start, length);
    sub[length] = '\0';

    return sub;
}

static void free_parts(char **parts, int count) {
    if (parts == NULL) {
        return;
    }

    for (int i = 0; i < count; i++) {
        free(parts[i]);
    }
    free(parts);
}

char** splitAndReverse(const char* str, int* count) {
    char* copy = strdup(str);
    char* token;
    int capacity = 10;

    char** result = malloc(capacity * sizeof(char*));
    *count = 0;

    token = strtok(copy, "*");
    while (token != NULL) {
        result[*count] = strdup(token);  // store token
        (*count)++;
        token = strtok(NULL, "*");
    }

    // reverse the array
    for (int i = 0; i < *count / 2; i++) {
        char* temp = result[i];
        result[i] = result[*count - i - 1];
        result[*count - i - 1] = temp;
    }

    free(copy);
    return result;
}

// extern void loadAndInterpret(char* filename) { 
//     if (filename == NULL) {
//         printf("[ERROR] Filename is NULL. Cannot proceed.\n");
//         return;
//     }
    
//     char* fileContent = readFile(filename); 
//     if (fileContent == NULL) {
//         printf("[ERROR] Could not load %s - readFile returned NULL\n", filename);
//         printf("[ERROR] File may not exist or read permission denied.\n");
//         return;
//     }
    
//     CountLines(fileContent);
//     parseInstructionsIntoMemory(fileContent);
    
//     free(fileContent);
// }

void callSemWait(Process *process , int resourceType){
    printf("SemWait: PID %d, Res %d\n", process->pcb->pid, resourceType);
    semWait(process, (enum RESOURCE) resourceType);
    printf("SemWait done\n");
}

void callSemSignal(int resourceType){
    printf("SemSignal: Res %d\n", resourceType);
    semSignal((enum RESOURCE) resourceType);
    printf("SemSignal done\n");
}

void callAssign(int pid , char* varName, char* varValue){
    printf("Assign: PID %d, %s = %s\n", pid, varName, varValue);
    writeToMemory(pid, varName, varValue);
    printf("Assign done\n");
}

void callPrint(char* data){
    printf("Print: %s\n", data);
    printData(data);
    printf("Print done\n");
}

void callPrintFromTo(int from, int to){
    printf("PrintFromTo: %d to %d\n", from, to);
    while(from<=to){
        char buffer[20];
        sprintf(buffer, "%d", from);
        printData(buffer);
        from++;
    }
    printf("PrintFromTo done\n");
}

static int is_integer_token(const char *value) {
    if (value == NULL || *value == '\0') {
        return 0;
    }

    const unsigned char *p = (const unsigned char *)value;
    if (*p == '-' || *p == '+') {
        p++;
    }

    if (*p == '\0') {
        return 0;
    }

    while (*p != '\0') {
        if (!isdigit(*p)) {
            return 0;
        }
        p++;
    }

    return 1;
}

static int resolve_int_arg(Process *process, const char *token, int *out_value) {
    if (is_integer_token(token)) {
        *out_value = atoi(token);
        return 1;
    }

    char *value = readFromMemory(process->pcb->pid, (char *)token);
    if (value == NULL || !is_integer_token(value)) {
        return 0;
    }

    *out_value = atoi(value);
    return 1;
}

static const char *resolve_string_arg(Process *process, const char *token) {
    char *value = readFromMemory(process->pcb->pid, (char *)token);
    if (value != NULL && value[0] != '\0') {
        return value;
    }

    return token;
}

void callWriteFile(char* filename, char* content){
    printf("WriteFile: %s\n", filename);
    writeFile(filename, content);
    printf("WriteFile done\n");
}

char* callReadFile(char* filename){
    printf("ReadFile: %s\n", filename);
    char* result = readFile(filename);
    printf("ReadFile: got %s\n", result);
    return result;
}

char* callTakeInput(){
    printf("TakeInput\n");
    char* result = takeInput();
    printf("Input: %s\n", result);
    return result;
}

void execute_instruction(Process* process) { 
    printf("Exec: PID %d\n", process->pcb->pid);
    if (process == NULL || process->pcb == NULL) {
        printf("Exec: NULL process\n");
        return;
    }

    printf("Exec: PC %d\n", process->pcb->pc);
    char* instruction = readInstruction(process->pcb->pc);

    if (instruction == NULL) {
        printf("Exec: No instr, FINISHED\n");
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        printf("Execution Error: No instruction found at PC %d\n", process->pcb->pc);
        return;
    }

    printf("Exec: Instr '%s'\n", instruction);
    int count = 0;
    char** parts = splitAndReverse(instruction, &count);

    printf("Exec: Parts\n");
    for (int i = 0; i < count; i++) {
        printf("Exec: Part[%d] '%s'\n", i, parts[i]);
        char* part = parts[i];
    
        if (strcmp(part, "000") == 0 ){
            if (i>=1){
                callSemWait(process, atoi(parts[i-1]));
            }
            else {
                printf("Syntax Error: Missing resource type for semWait in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "001") == 0 ){
            if (i>=1){
                callSemSignal(atoi(parts[i-1]));
            }
            else {
                printf("Syntax Error: Missing resource type for semSignal in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "010") == 0 ){
            if (i>=2){
                callAssign(process->pcb->pid, parts[i-1], parts[i-2]);
            }
            else {
                printf("Syntax Error: Missing variable name or value for assign in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "011") == 0 ){
            if (i>=1){
                const char *value = resolve_string_arg(process, parts[i-1]);
                callPrint((char *)value);
            }
            else {
                printf("Syntax Error: Missing data to print for print in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "100") == 0 ){
            if (i>=2){
                int from = 0;
                int to = 0;
                if (!resolve_int_arg(process, parts[i-1], &from) ||
                    !resolve_int_arg(process, parts[i-2], &to)) {
                    printf("Runtime Error: printFromTo arguments must be integers or variables with integer values\n");
                } else {
                    callPrintFromTo(from, to);
                }
            }
            else {
                printf("Syntax Error: Missing range values for printFromTo in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "101") == 0 ){
            if (i>=2){
                const char *filename = resolve_string_arg(process, parts[i-1]);
                const char *content = resolve_string_arg(process, parts[i-2]);
                callWriteFile((char *)filename, (char *)content);
            }
            else {
                printf("Syntax Error: Missing filename or content for writeFile in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "110") == 0 || strcmp(part, "readFile") == 0 ){
            if (i>=1){
                const char *filename = resolve_string_arg(process, parts[i-1]);
                parts[i] = callReadFile((char *)filename);
            }
            else {
                printf("Syntax Error: Missing filename for readFile in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "input") == 0 ){
            parts[i] = callTakeInput();
        }
    }
    
    if (process->pcb->pc >= process->pcb->memory_bounds[1]) {
        printf("Exec: Reached memory boundary, finishing\n");
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
    } else {
        process->pcb->pc += 1;
        printf("Exec: Update PC\n");
        update_pc_in_memory(process->pcb->pid, process->pcb->pc);
    }
    printf("Exec: Free parts\n");
    free_parts(parts, count);
    printf("Exec: Done\n");
}




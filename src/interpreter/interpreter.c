#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../synchronization/mutex.h"
#include "parser.h"

int global_pid ;

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
    printData(readFromMemory(global_pid, data));
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

void callWriteFile(char* filename, char* content){
    printf("WriteFile: %s\n", filename);
    writeFile(readFromMemory(global_pid, filename), readFromMemory(global_pid, content));
    printf("WriteFile done\n");
}

char* callReadFile(char* filename){
    printf("ReadFile: %s\n", filename);
    char* result = readFile(readFromMemory(global_pid, filename));
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

    global_pid = process->pcb->pid;

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
                callPrint(parts[i-1]);
            }
            else {
                printf("Syntax Error: Missing data to print for print in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "100") == 0 ){
            if (i>=2){
                char* from_str = readFromMemory(global_pid, parts[i-1]);
                char* to_str = readFromMemory(global_pid, parts[i-2]);
                printf("Exec: printFromTo from_var='%s' from_val='%s', to_var='%s' to_val='%s'\n", 
                       parts[i-1], from_str ? from_str : "NULL", 
                       parts[i-2], to_str ? to_str : "NULL");
                int from = from_str ? atoi(from_str) : 0;
                int to = to_str ? atoi(to_str) : 0;
                printf("Exec: printFromTo from=%d, to=%d\n", from, to);
                callPrintFromTo(from, to);
            }
            else {
                printf("Syntax Error: Missing range values for printFromTo in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "101") == 0 ){
            if (i>=2){
                callWriteFile(parts[i-1], parts[i-2]);
            }
            else {
                printf("Syntax Error: Missing filename or content for writeFile in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "110") == 0 ){
            if (i>=1){
                parts[i] = callReadFile(parts[i-1]);
            }
            else {
                printf("Syntax Error: Missing filename for readFile in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "input") == 0 ){
            parts[i] = callTakeInput();
        }
    }
    
//one?
    process->pcb->pc += 1;
    printf("Exec: Update PC\n");
    update_pc_in_memory(process->pcb->pid, process->pcb->pc);
    printf("Exec: Free parts\n");
    free_parts(parts, count);
    printf("Exec: Done\n");
}




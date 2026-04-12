#include <stdio.h>
#include <stdlib.h>
#include "syscalls.h"  // For readFile
#include "../memory/memoryy.h"    // For your memory functions
#include "parser.h"    // For your parsing logic
#include "pcb.h"    // For PCB structure
#include "mutex.h"  // For semaphore operations

char* substring(const char* src, int start, int length) {
    char* sub = malloc(length + 1);   // +1 for '\0'
    if (sub == NULL) return NULL;

    strncpy(sub, src + start, length);
    sub[length] = '\0';

    return sub;   // caller must free()
}



char** splitAndReverse(const char* str, int* count) {
    char* copy = strdup(str);   // make modifiable copy
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


void callSemWait(PCB * process , int resourceType){
    semWait(process, resourceType);
}

void callSemSignal(int resourceType){
    semSignal(resourceType);
}

void callAssign(int pid , char* varName, char* varValue){
    writeToMemory(pid, varName, varValue);
}

void callPrint(char* data){
    printData(data);
}

void callPrintFromTo(int from, int to){
    while(from<=to){
        char buffer[20];
        sprintf(buffer, "%d", from);
        printData(buffer);
        from++;
    }
}

void callWriteFile(char* filename, char* content){
    writeFile(filename, content);
}

char* callReadFile(char* filename){
    return readFile(filename);
}

char* callTakeInput(){
    return takeInput();
}

void execute_instruction(PCB* process) { 

    char* instruction = readFromMemory(process->pc);

    if (instruction == NULL) {
        printf("Execution Error: No instruction found at PC %d\n", process->pc);
        return;
    }

    int count;

    char** parts = splitAndReverse(instruction, &count);

    for (int i =0 ; i < count ; i++){

        char* part = parts[i];
    
        if (strcmp(part, "000") == 0 ){
            if (i>=1){
                callSemWait(process, parts[i-1]);
            }
            else {
                printf("Syntax Error: Missing resource type for semWait in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "001") == 0 ){
            if (i>=1){
                callSemSignal(parts[i-1]);
            }
            else {
                printf("Syntax Error: Missing resource type for semSignal in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "010") == 0 ){
            if (i>=2){
                callAssign(process->pid, parts[i-1], parts[i-2]);
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
                int from = atoi(parts[i-1]);
                int to = atoi(parts[i-2]);
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
    
    printf("Executing instruction at PC %d: %s\n", process->pc, instruction);

    process -> pc += sizeof(instruction); 
}




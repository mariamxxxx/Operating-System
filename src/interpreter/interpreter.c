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
static int parse_resource_token(const char *token) {
    if (token == NULL) {
        return -1;
    }

    if (strcmp(token, "0") == 0 || strcmp(token, "userInput") == 0 || strcmp(token, "USER_INPUT") == 0) {
        return 0;
    }

    if (strcmp(token, "1") == 0 || strcmp(token, "userOutput") == 0 || strcmp(token, "USER_OUTPUT") == 0) {
        return 1;
    }

    if (strcmp(token, "2") == 0 || strcmp(token, "file") == 0 || strcmp(token, "fileResource") == 0 || strcmp(token, "FILE_RESOURCE") == 0) {
        return 2;
    }

    return -1;
}



void callSemWait(Process *process , int resourceType){
    printf("=================================================\n");
    printf("SemWait: PID %d, Res %d\n", process->pcb->pid, resourceType);
    semWait(process, (enum RESOURCE) resourceType);
    printf("SemWait done\n");
    printf("=================================================\n");
}

void callSemSignal(int resourceType){
    printf("=================================================\n");
    printf("SemSignal: Res %d\n", resourceType);
    semSignal((enum RESOURCE) resourceType);
    printf("SemSignal done\n");
    printf("=================================================\n");
}

void callAssign(int pid , char* varName, char* varValue){
    printf("=================================================\n");
    printf("Assign: PID %d, %s = %s\n", pid, varName, varValue);
    if (varValue == NULL) {
        printf("Assign: NULL value, skipping assignment\n");
        return;
    }
    writeToMemory(pid, varName, varValue);
    printf("Assign done\n");
    printf("=================================================\n");
}

void callPrint(char* data){
    printf("=================================================\n");
    printf("Print: %s\n", data);
    if (data == NULL) {
        printData("(null)");
    } else {
        printData(readFromMemory(global_pid, data));
    }
    printf("Print done\n");
    printf("=================================================\n");
}

void callPrintFromTo(int from, int to){
    printf("=================================================\n");
    printf("PrintFromTo: %d to %d\n", from, to);
    while(from<=to){
        char buffer[20];
        sprintf(buffer, "%d", from);
        printData(buffer);
        from++;
    }
    printf("PrintFromTo done\n");
    printf("=================================================\n");
}

void callWriteFile(char* filename, char* content){
    printf("=================================================\n");
    printf("WriteFile: %s\n", filename);
    char* contentValue = readFromMemory(global_pid, content);
    if (contentValue == NULL) {
        printf("Variable %s not found, skipping writeFile\n", content);
        return;
    }
    writeFile( readFromMemory(global_pid, filename), contentValue);
    printf("WriteFile done\n");
    printf("=================================================\n");
}

char* callReadFile(char* filename){
    printf("=================================================\n");
    char* resolved_filename = readFromMemory(global_pid, filename);
    if (resolved_filename == NULL || resolved_filename[0] == '\0') {
        printf("ReadFile: variable %s not found or empty for process id %d\n", filename, global_pid);
        return strdup("");
    }
    printf("ReadFile: %s\n", resolved_filename);
    char* result = readFile(resolved_filename);
    if (result == NULL) {
        printf("ReadFile: could not open file %s\n", resolved_filename);
        return strdup("");
    }
    printf("ReadFile: got file content\n");
    printf("=================================================\n");
    return result;

}

char* callTakeInput(){
    printf("=================================================\n");
    printf("TakeInput\n");
    char* result = takeInput();
    printf("Input: %s\n", result);
    printf("=================================================\n");
    return result;
}

void execute_instruction(Process* process) { 

    global_pid = process->pcb->pid;

    printf("Exec: PID %d\n", process->pcb->pid);
    if (process == NULL || process->pcb == NULL) {
    printf("[ERROR] Attempted to execute a NULL process.\n");        
    return;
    }

    printf("\n=================================================\n");
    printf("[RUN] Executing Process PID = %d | PC = %d\n",process->pcb->pid, process->pcb->pc);
    printf("=================================================\n");    char* instruction = readInstruction(process->pcb->pc);

    if (instruction == NULL) {
        printf("[INFO] No instruction found. Process %d has finished execution.\n",process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }

    printf("[FETCH] Instruction: \"%s\"\n", instruction);
    int count = 0;
    char** parts = splitAndReverse(instruction, &count);

    for (int i = 0; i < count; i++) {
        printf("[DECODE] Instruction parts (processed right-to-left):\n");
        printf("  -> Part[%d]: %s\n", i, parts[i]);
        char* part = parts[i];

        printf("\n[EXECUTION]\n");
    
        if (strcmp(part, "000") == 0 ){
            if (i>=1){
                // callSemWait(process, atoi(parts[i-1]));
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType == 0 || resourceType == 1 || resourceType == 2)  {
                    callSemWait(process, resourceType);
                } else {
                    printf("Syntax Error: Invalid resource type '%s' for semWait in instruction %s\n", parts[i-1], instruction);
                }
            }
            else {
                printf("Syntax Error: Missing resource type for semWait in instruction %s\n", instruction);
            }
        }
        if (strcmp(part, "001") == 0 ){
            if (i>=1){
                // callSemSignal(atoi(parts[i-1]));
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType == 0 || resourceType == 1 || resourceType == 2)  {
                    callSemSignal(resourceType);
                } else {
                    printf("Syntax Error: Invalid resource type '%s' for semSignal in instruction %s\n", parts[i-1], instruction);
                }
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
            printf("Process %d is goint to take input, please press enter \n", process->pcb->pid);
            parts[i] = callTakeInput();
        }
    }
    
//one?
    process->pcb->pc += 1;
    // printf("Exec: Update PC\n");
    update_pc_in_memory(process->pcb->pid, process->pcb->pc);
    // printf("Exec: Free parts\n");
    free_parts(parts, count);
    // printf("Exec: Done\n");

    char* next_instruction = readInstruction(process->pcb->pc);

    if (next_instruction == NULL) {
printf("\n[INFO] Process %d has completed all instructions.\n",process->pcb->pid);        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }

    printf("=================================================\n");
    printf("[DONE] End of instruction for PID %d\n", process->pcb->pid);
    printf("=================================================\n");
}



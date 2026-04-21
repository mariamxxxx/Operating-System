#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../synchronization/mutex.h"
#include "parser.h"

// Link to the GUI logger
extern void gui_log(const char* format, ...);

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
    gui_log("SemWait: PID %d, Res %d", process->pcb->pid, resourceType);
    semWait(process, (enum RESOURCE) resourceType);
    gui_log("SemWait done");
}

void callSemSignal(int resourceType){
    gui_log("SemSignal: Res %d", resourceType);
    semSignal((enum RESOURCE) resourceType);
    gui_log("SemSignal done");
}

void callAssign(int pid , char* varName, char* varValue){
    gui_log("Assign: PID %d, %s = %s", pid, varName, varValue);
    if (varValue == NULL) {
        gui_log("Assign: NULL value, skipping assignment");
        return;
    }
    writeToMemory(pid, varName, varValue);
    gui_log("Assign done");
}

void callPrint(char* data){
    gui_log("Print: %s", data);
    if (data == NULL) {
        printData("(null)");
    } else {
        printData(readFromMemory(global_pid, data));
    }
    gui_log("Print done");
}

void callPrintFromTo(int from, int to){
    gui_log("PrintFromTo: %d to %d", from, to);
    while(from<=to){
        char buffer[20];
        sprintf(buffer, "%d", from);
        printData(buffer);
        from++;
    }
    gui_log("PrintFromTo done");
}

void callWriteFile(char* filename, char* content){
    gui_log("WriteFile: %s", filename);
    char* contentValue = readFromMemory(global_pid, content);
    if (contentValue == NULL) {
        gui_log("Variable %s not found, skipping writeFile", content);
        return;
    }
    writeFile( readFromMemory(global_pid, filename), contentValue);
    gui_log("WriteFile done");
}

char* callReadFile(char* filename){
    gui_log("ReadFile: %s", filename);
    char* result = readFromMemory(global_pid, filename);
    if (result == NULL) {
        gui_log("Variable %s not found in memory for process id %d", filename, global_pid);
        return strdup("");  // Return malloced empty string
    }
    gui_log("ReadFile: got %s", result);
    return strdup(result);  // Return malloced copy
}

char* callTakeInput(){
    gui_log("TakeInput");
    char* result = takeInput();
    gui_log("Input: %s", result);
    return result;
}

// [Keep the top of your parser.c identical up to execute_instruction]

extern bool input_submitted; // Link to the GUI flag

void execute_instruction(Process* process) { 
    global_pid = process->pcb->pid;

    gui_log("Exec: PID %d", process->pcb->pid);
    if (process == NULL || process->pcb == NULL) {
        gui_log("Exec: NULL process");
        return;
    }

    gui_log("Exec: PC %d", process->pcb->pc);
    
    if (process->pcb->pc < process->pcb->memory_bounds[0] + 7 ||
        process->pcb->pc > process->pcb->memory_bounds[1]) {
        gui_log("Exec: PC %d out of process bounds [%d, %d], marking FINISHED",
               process->pcb->pc,
               process->pcb->memory_bounds[0],
               process->pcb->memory_bounds[1]);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }

    char* instruction = readInstruction(process->pcb->pc);

    if (instruction == NULL) {
        gui_log("Exec: No instr, FINISHED");
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        gui_log("Execution Error: No instruction found at PC %d", process->pcb->pc);
        return;
    }

    gui_log("Exec: Instr '%s'", instruction);
    int count = 0;
    char** parts = splitAndReverse(instruction, &count);

    gui_log("Exec: Parts");
    for (int i = 0; i < count; i++) {
        gui_log("Exec: Part[%d] '%s'", i, parts[i]);
        char* part = parts[i];
    
        if (strcmp(part, "000") == 0 ){
            if (i>=1){
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType >= 0) { callSemWait(process, resourceType); } 
                else { gui_log("Syntax Error: Invalid resource type '%s' for semWait in instruction %s", parts[i-1], instruction); }
            }
            else { gui_log("Syntax Error: Missing resource type for semWait in instruction %s", instruction); }
        }
        if (strcmp(part, "001") == 0 ){
            if (i>=1){
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType >= 0) { callSemSignal(resourceType); } 
                else { gui_log("Syntax Error: Invalid resource type '%s' for semSignal in instruction %s", parts[i-1], instruction); }
            }
            else { gui_log("Syntax Error: Missing resource type for semSignal in instruction %s", instruction); }
        }
        if (strcmp(part, "010") == 0 ){
            if (i>=2){ callAssign(process->pcb->pid, parts[i-1], parts[i-2]); }
            else { gui_log("Syntax Error: Missing variable name or value for assign in instruction %s", instruction); }
        }
        if (strcmp(part, "011") == 0 ){
            if (i>=1){ callPrint(parts[i-1]); }
            else { gui_log("Syntax Error: Missing data to print for print in instruction %s", instruction); }
        }
        if (strcmp(part, "100") == 0 ){
            if (i>=2){
                char* from_str = readFromMemory(global_pid, parts[i-1]);
                char* to_str = readFromMemory(global_pid, parts[i-2]);
                gui_log("Exec: printFromTo from_var='%s' from_val='%s', to_var='%s' to_val='%s'", 
                       parts[i-1], from_str ? from_str : "NULL", parts[i-2], to_str ? to_str : "NULL");
                int from = from_str ? atoi(from_str) : 0;
                int to = to_str ? atoi(to_str) : 0;
                gui_log("Exec: printFromTo from=%d, to=%d", from, to);
                callPrintFromTo(from, to);
            }
            else { gui_log("Syntax Error: Missing range values for printFromTo in instruction %s", instruction); }
        }
        if (strcmp(part, "101") == 0 ){
            if (i>=2){ callWriteFile(parts[i-1], parts[i-2]); }
            else { gui_log("Syntax Error: Missing filename or content for writeFile in instruction %s", instruction); }
        }
        if (strcmp(part, "110") == 0 ){
            if (i>=1){ parts[i] = callReadFile(parts[i-1]); }
            else { gui_log("Syntax Error: Missing filename for readFile in instruction %s", instruction); }
        }
        if (strcmp(part, "input") == 0 ){
            // FIX: If the user hasn't hit ENTER yet, abort and try again next tick
            if (!input_submitted) {
                gui_log("Process %d is goint to take input, please press enter ", process->pcb->pid);
                free_parts(parts, count);
                return; // Return WITHOUT advancing the Program Counter (PC)!
            } else {
                // If they have hit enter, consume the input and let the instruction finish!
                parts[i] = callTakeInput();
                input_submitted = false; // Reset the flag
            }
        }
    }
    
    // PC only increments if the instruction completes successfully without aborting
    process->pcb->pc += 1;
    gui_log("Exec: Update PC");
    update_pc_in_memory(process->pcb->pid, process->pcb->pc);
    gui_log("Exec: Free parts");
    free_parts(parts, count);
    gui_log("Exec: Done");
    
    //gui
    if (process->pcb->pc > process->pcb->memory_bounds[1]) {
        gui_log("Process %d FINISHED (reached end of memory bounds)", process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }
//
    char* next_instruction = readInstruction(process->pcb->pc);

    if (next_instruction == NULL) {
        gui_log("Process %d FINISHED", process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }
}

// void execute_instruction(Process* process) { 

//     global_pid = process->pcb->pid;

//     gui_log("Exec: PID %d", process->pcb->pid);
//     if (process == NULL || process->pcb == NULL) {
//         gui_log("Exec: NULL process");
//         return;
//     }

//     gui_log("Exec: PC %d", process->pcb->pc);
    
//     //gui
//     // Guard against stale/corrupted PC values when processes are swapped frequently. 
//     if (process->pcb->pc < process->pcb->memory_bounds[0] + 7 ||
//         process->pcb->pc > process->pcb->memory_bounds[1]) {
//         gui_log("Exec: PC %d out of process bounds [%d, %d], marking FINISHED",
//                process->pcb->pc,
//                process->pcb->memory_bounds[0],
//                process->pcb->memory_bounds[1]);
//         process->pcb->state = FINISHED;
//         update_state_in_memory(process->pcb->pid, FINISHED);
//         return;
//     }

//     char* instruction = readInstruction(process->pcb->pc);

//     if (instruction == NULL) {
//         gui_log("Exec: No instr, FINISHED");
//         process->pcb->state = FINISHED;
//         update_state_in_memory(process->pcb->pid, FINISHED);
//         gui_log("Execution Error: No instruction found at PC %d", process->pcb->pc);
//         return;
//     }

//     gui_log("Exec: Instr '%s'", instruction);
//     int count = 0;
//     char** parts = splitAndReverse(instruction, &count);

//     gui_log("Exec: Parts");
//     for (int i = 0; i < count; i++) {
//         gui_log("Exec: Part[%d] '%s'", i, parts[i]);
//         char* part = parts[i];
    
//         if (strcmp(part, "000") == 0 ){
//             if (i>=1){
//                 // callSemWait(process, atoi(parts[i-1]));
//                 int resourceType = parse_resource_token(parts[i-1]);
//                 if (resourceType >= 0) {
//                     callSemWait(process, resourceType);
//                 } else {
//                     gui_log("Syntax Error: Invalid resource type '%s' for semWait in instruction %s", parts[i-1], instruction);
//                 }
//             }
//             else {
//                 gui_log("Syntax Error: Missing resource type for semWait in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "001") == 0 ){
//             if (i>=1){
//                 // callSemSignal(atoi(parts[i-1]));
//                 int resourceType = parse_resource_token(parts[i-1]);
//                 if (resourceType >= 0) {
//                     callSemSignal(resourceType);
//                 } else {
//                     gui_log("Syntax Error: Invalid resource type '%s' for semSignal in instruction %s", parts[i-1], instruction);
//                 }
//             }
//             else {
//                 gui_log("Syntax Error: Missing resource type for semSignal in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "010") == 0 ){
//             if (i>=2){
//                 callAssign(process->pcb->pid, parts[i-1], parts[i-2]);
//             }
//             else {
//                 gui_log("Syntax Error: Missing variable name or value for assign in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "011") == 0 ){
//             if (i>=1){
//                 callPrint(parts[i-1]);
//             }
//             else {
//                 gui_log("Syntax Error: Missing data to print for print in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "100") == 0 ){
//             if (i>=2){
//                 char* from_str = readFromMemory(global_pid, parts[i-1]);
//                 char* to_str = readFromMemory(global_pid, parts[i-2]);
//                 gui_log("Exec: printFromTo from_var='%s' from_val='%s', to_var='%s' to_val='%s'", 
//                        parts[i-1], from_str ? from_str : "NULL", 
//                        parts[i-2], to_str ? to_str : "NULL");
//                 int from = from_str ? atoi(from_str) : 0;
//                 int to = to_str ? atoi(to_str) : 0;
//                 gui_log("Exec: printFromTo from=%d, to=%d", from, to);
//                 callPrintFromTo(from, to);
//             }
//             else {
//                 gui_log("Syntax Error: Missing range values for printFromTo in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "101") == 0 ){
//             if (i>=2){
//                 callWriteFile(parts[i-1], parts[i-2]);
//             }
//             else {
//                 gui_log("Syntax Error: Missing filename or content for writeFile in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "110") == 0 ){
//             if (i>=1){
//                 parts[i] = callReadFile(parts[i-1]);
                
//             }
//             else {
//                 gui_log("Syntax Error: Missing filename for readFile in instruction %s", instruction);
//             }
//         }
//         if (strcmp(part, "input") == 0 ){
//             gui_log("Process %d is goint to take input, please press enter ", process->pcb->pid);
//             parts[i] = callTakeInput();
//         }
//     }
    
// //one?
//     process->pcb->pc += 1;
//     gui_log("Exec: Update PC");
//     update_pc_in_memory(process->pcb->pid, process->pcb->pc);
//     gui_log("Exec: Free parts");
//     free_parts(parts, count);
//     gui_log("Exec: Done");
    
//     //gui
//     if (process->pcb->pc > process->pcb->memory_bounds[1]) {
//         gui_log("Process %d FINISHED (reached end of memory bounds)", process->pcb->pid);
//         process->pcb->state = FINISHED;
//         update_state_in_memory(process->pcb->pid, FINISHED);
//         return;
//     }
// //
//     char* next_instruction = readInstruction(process->pcb->pc);

//     if (next_instruction == NULL) {
//         gui_log("Process %d FINISHED", process->pcb->pid);
//         process->pcb->state = FINISHED;
//         update_state_in_memory(process->pcb->pid, FINISHED);
//         return;
//     }
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../memory/memoryy.h"
#include "../os/syscalls.h"
#include "../synchronization/mutex.h"
#include "parser.h"

#ifdef GUI_MODE
// Link to the GUI logger
extern void gui_log(const char *format, ...);
#define LOGF(...) gui_log(__VA_ARGS__)
#else
#define LOGF(...) printf(__VA_ARGS__)
#endif

// 1: last execute bailed on "assign … input" before submit; sim clock/quantum should not advance
static int s_instruction_stalled_on_input = 0;

void instruction_clear_stall(void) {
    s_instruction_stalled_on_input = 0;
}

int instruction_stalled_on_input(void) {
    return s_instruction_stalled_on_input;
}

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
    LOGF("SemWait: PID %d, Res %d\n", process->pcb->pid, resourceType);
    semWait(process, (enum RESOURCE) resourceType);
    LOGF("SemWait done\n");
    printf("=================================================\n");
}

void callSemSignal(int resourceType){
    printf("=================================================\n");
    LOGF("SemSignal: Res %d\n", resourceType);
    semSignal((enum RESOURCE) resourceType);
    LOGF("SemSignal done\n");
    printf("=================================================\n");
}

void callAssign(int pid , char* varName, char* varValue){
    printf("=================================================\n");
    LOGF("Assign: PID %d, %s = %s\n", pid, varName, varValue);
    if (varValue == NULL) {
        LOGF("Assign: NULL value, skipping assignment\n");
        return;
    }
    writeToMemory(pid, varName, varValue);
    LOGF("Assign done\n");
    printf("=================================================\n");
}

void callPrint(char* data){
    printf("=================================================\n");
    LOGF("Print: %s\n", data);
    if (data == NULL) {
        printData("(null)");
    } else {
        printData(readFromMemory(global_pid, data));
    }
    LOGF("Print done\n");
    printf("=================================================\n");
}

void callPrintFromTo(int from, int to){
    printf("=================================================\n");
    LOGF("PrintFromTo: %d to %d\n", from, to);
    while(from<=to){
        char buffer[20];
        sprintf(buffer, "%d", from);
        printData(buffer);
        from++;
    }
    LOGF("PrintFromTo done\n");
    printf("=================================================\n");
}

void callWriteFile(char* filename, char* content){
    printf("=================================================\n");
    LOGF("WriteFile: %s\n", filename);
    char* contentValue = readFromMemory(global_pid, content);
    if (contentValue == NULL) {
        LOGF("Variable %s not found, skipping writeFile\n", content);
        return;
    }
    writeFile( readFromMemory(global_pid, filename), contentValue);
    LOGF("WriteFile done\n");
    printf("=================================================\n");
}

char* callReadFile(char* filename){
    printf("=================================================\n");
    char* resolved_filename = readFromMemory(global_pid, filename);
    if (resolved_filename == NULL || resolved_filename[0] == '\0') {
        LOGF("ReadFile: variable %s not found or empty for process id %d\n", filename, global_pid);
        return strdup("");
    }
    printf("ReadFile: %s\n", resolved_filename);
    char* result = readFile(resolved_filename);
    if (result == NULL) {
        LOGF("ReadFile: could not open file %s\n", resolved_filename);
        return strdup("");
    }
    LOGF("ReadFile: got file content\n");
    printf("=================================================\n");
    return result;

}

char* callTakeInput(){
    printf("=================================================\n");
    LOGF("TakeInput\n");
    char* result = takeInput();
    LOGF("Input: %s\n", result);
    printf("=================================================\n");
    return result;
}

// [Keep the top of your parser.c identical up to execute_instruction]

#ifdef GUI_MODE
extern bool input_submitted; // Link to the GUI flag
#endif

void execute_instruction(Process* process) { 
    instruction_clear_stall();
    global_pid = process->pcb->pid;

    LOGF("Exec: PID %d\n", process->pcb->pid);
    if (process == NULL || process->pcb == NULL) {
        printf("[ERROR] Attempted to execute a NULL process.\n");
        LOGF("Exec: NULL process\n");
        return;
    }

    LOGF("Exec: PC %d\n", process->pcb->pc);
    
    if (process->pcb->pc < process->pcb->memory_bounds[0] + 7 ||
        process->pcb->pc > process->pcb->memory_bounds[1]) {
        LOGF("Exec: PC %d out of process bounds [%d, %d], marking FINISHED\n",
               process->pcb->pc,
               process->pcb->memory_bounds[0],
               process->pcb->memory_bounds[1]);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }

    printf("\n=================================================\n");
    printf("[RUN] Executing Process PID = %d | PC = %d\n",process->pcb->pid, process->pcb->pc);
    printf("=================================================\n");  
    char* instruction = readInstruction(process->pcb->pc);

    if (instruction == NULL) {
        LOGF("[INFO] No instruction found. Process %d has finished execution.\n",process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        LOGF("Execution Error: No instruction found at PC %d\n", process->pcb->pc);
        return;
    }

    LOGF("[FETCH] Instruction: \"%s\"\n", instruction);
    int count = 0;
    char** parts = splitAndReverse(instruction, &count);

    LOGF("Exec: Parts\n");
    for (int i = 0; i < count; i++) {
        LOGF("[DECODE] Instruction parts (processed right-to-left):\n");
        printf("  -> Part[%d]: %s", i, parts[i]);
        char* part = parts[i];

        printf("\n[EXECUTION]\n");
    
        if (strcmp(part, "000") == 0 ){
            if (i>=1){
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType == 0 || resourceType == 1 || resourceType == 2)  { callSemWait(process, resourceType); } 
                else { LOGF("Syntax Error: Invalid resource type '%s' for semWait in instruction %s\n", parts[i-1], instruction); }
            }
            else { LOGF("Syntax Error: Missing resource type for semWait in instruction %s\n", instruction); }
        }
        if (strcmp(part, "001") == 0 ){
            if (i>=1){
                int resourceType = parse_resource_token(parts[i-1]);
                if (resourceType == 0 || resourceType == 1 || resourceType == 2)  { callSemSignal(resourceType); } 
                else { LOGF("Syntax Error: Invalid resource type '%s' for semSignal in instruction %s\n", parts[i-1], instruction); }
            }
            else { LOGF("Syntax Error: Missing resource type for semSignal in instruction %s\n", instruction); }
        }
        if (strcmp(part, "010") == 0 ){
            if (i>=2){ callAssign(process->pcb->pid, parts[i-1], parts[i-2]); }
            else { LOGF("Syntax Error: Missing variable name or value for assign in instruction %s\n", instruction); }
        }
        if (strcmp(part, "011") == 0 ){
            if (i>=1){ callPrint(parts[i-1]); }
            else { LOGF("Syntax Error: Missing data to print for print in instruction %s\n", instruction); }
        }
        if (strcmp(part, "100") == 0 ){
            if (i>=2){
                char* from_str = readFromMemory(global_pid, parts[i-1]);
                char* to_str = readFromMemory(global_pid, parts[i-2]);
                LOGF("Exec: printFromTo from_var='%s' from_val='%s', to_var='%s' to_val='%s'\n", 
                       parts[i-1], from_str ? from_str : "NULL", parts[i-2], to_str ? to_str : "NULL");
                int from = from_str ? atoi(from_str) : 0;
                int to = to_str ? atoi(to_str) : 0;
                LOGF("Exec: printFromTo from=%d, to=%d\n", from, to);
                callPrintFromTo(from, to);
            }
            else { LOGF("Syntax Error: Missing range values for printFromTo in instruction %s\n", instruction); }
        }
        if (strcmp(part, "101") == 0 ){
            if (i>=2){ callWriteFile(parts[i-1], parts[i-2]); }
            else { LOGF("Syntax Error: Missing filename or content for writeFile in instruction %s\n", instruction); }
        }
        if (strcmp(part, "110") == 0 ){
            if (i>=1){ parts[i] = callReadFile(parts[i-1]); }
            else { LOGF("Syntax Error: Missing filename for readFile in instruction %s\n", instruction); }
        }
        if (strcmp(part, "input") == 0 ){
#ifdef GUI_MODE
            // If the user hasn't hit ENTER yet, abort and try again; one logical clock
            // tick is charged only when the instruction completes (see os_step / schedulers).
            if (!input_submitted) {
                LOGF("Process %d is goint to take input, please press enter \n", process->pcb->pid);
                free_parts(parts, count);
                s_instruction_stalled_on_input = 1;
                return; // Return WITHOUT advancing the Program Counter (PC)!
            } else {
                // If they have hit enter, consume the input and let the instruction finish!
                parts[i] = callTakeInput();
                input_submitted = false; // Reset the flag
            }
#else
            parts[i] = callTakeInput();
#endif
        }
    }
    
    // PC only increments if the instruction completes successfully without aborting
    process->pcb->pc += 1;
    // gui_log("Exec: Update PC");
    update_pc_in_memory(process->pcb->pid, process->pcb->pc);
    // gui_log("Exec: Free parts");
    free_parts(parts, count);
    // gui_log("Exec: Done");
    
    //gui
    if (process->pcb->pc > process->pcb->memory_bounds[1]) {
        LOGF("Process %d FINISHED (reached end of memory bounds)\n", process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }
//
    char* next_instruction = readInstruction(process->pcb->pc);

    if (next_instruction == NULL) {
        LOGF("[INFO] Process %d has completed all instructions.\n",process->pcb->pid);
        process->pcb->state = FINISHED;
        update_state_in_memory(process->pcb->pid, FINISHED);
        return;
    }

    printf("=================================================\n");
    printf("[DONE] End of instruction for PID %d\n", process->pcb->pid);
    printf("=================================================\n");
}



#include <stdio.h>  // For FILE, fopen, printf, fgets, fprintf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen
#include "../process/processs.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/queue.h"
#include "../memory/memoryy.h"    // For your memory functions

int CountLines(char* rawData) {
    int countLines = 0 ;
    char* line = strtok(rawData, "\n"); // Get first line
    while (line != NULL) {
        countLines++;
        line = strtok(NULL, "\n"); // Get next line
    }
    return countLines;
}

int pid_int = 1; // Global variable to hold the PID for the process being initialized


char* AvailableFunctions [] = {
    "semWait", "semSignal", "assign", "print", "printFromTo","writeFile","readFile"
};
char* AvailableFunctionsOpcodes[] = {
    "000", "001", "010", "011", "100", "101", "110"
};
char* sem[] = {
    "0", "1", "2"
};

char* SplitInstruction(char* line) {
    char *tokens[4];
    int count = 0;

    char *token = strtok(line, " ");
    while (token != NULL && count < 4) {
        tokens[count++] = token;
        token = strtok(NULL, " ");
    }
    return tokens;
}

// int CountLines(char* rawData) {

//     int countLines =0;

//     char* line = strtok(rawData, "\n"); // Get first line
    
//     while (line != NULL) {

//         countLines++;

//         line = strtok(NULL, "\n"); // Get next line
//     }

//     return countLines;
// }

char* parseLineToOpcode(char* line) {
    char *Instruction[] = SplitInstruction(line);

    char *instructionName = Instruction[0];

    char result[100] ="";

    for (int i = 0; i < 7; i++) {
        if (strcmp(instructionName, AvailableFunctions[i]) == 0) {
            strcpy(result, AvailableFunctionsOpcodes[i]);  
        }
    }

    char *instructionParameter1 = Instruction[1];

    if (strcmp(instructionName, "000") == 0 || strcmp(instructionName, "001") == 0){
        strcat(result, "*");
        if (strcmp(instructionParameter1, "userInput") == 0) {
            strcpy(result, strcat(result,"0"));
    }
        else if (strcmp(instructionParameter1, "userOutput") == 0) {
            strcpy(result, strcat(result,"1"));
    }
        else if (strcmp(instructionParameter1, "file") == 0) {
            strcpy(result, strcat(result,"2"));
    }

    

    }
    else {
        if (instructionParameter1 != NULL) {
            strcat(result, "*");
            strcat(result, instructionParameter1);
        }

        if (Instruction[2] != NULL) {
            strcat(result, "*");
            strcat(result, Instruction[2]);
        }

        if (Instruction[3] != NULL) {
            strcat(result, "*");
            strcat(result, Instruction[3]);
        }
    }

    return result;
}

void parseInstructionsIntoMemory(char* rawData , Process* process) {

    char* line = strtok(rawData, "\n"); // Get first line

    int i = 0; 
    
    while (line != NULL) {

        char* instruction = parseLineToOpcode(line); 

        strcpy(process->code_lines[i], instruction);

        i++;

        process->code_line_count++;

        line = strtok(NULL, "\n"); // Get next line
    }

    allocate_memory(process->pcb->pid, process);

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

    Process * process = initProcess(pid_int); // Initialize process with PID and line count

    enqueue(&(process ->pcb), &os_ready_queue); // ma na5od el process kolahaaaa

    pid_int++; // Increment global PID for next process
    
    parseInstructionsIntoMemory(fileContent , process);
    
    free(fileContent);
}
#include <stdio.h>
#include <stdlib.h>
#include "syscalls.h"  // For readFile
#include "memory.h"    // For your memory functions
#include "parser.h"    // For your parsing logic
#include "pcb.h"    // For PCB structure


void loadAndInterpret(char* filename) {
    char* fileContent = readFile(filename);
    
    if (fileContent == NULL) {
        printf("Interpreter Error: Could not load %s\n", filename);
        return;
    }

    parseInstructionsIntoMemory(fileContent);

    free(fileContent);
    
    printf("Program %s successfully loaded into memory.\n", filename);
}


void execute_instruction(PCB* process) {

    char* instruction = readFromMemory(process->pc);
    
    if (instruction == NULL) {
        printf("Execution Error: No instruction found at PC %d\n", process->pc);
        return;
    }

    // Decode and execute the instruction
    // This is a placeholder. You would implement your actual instruction decoding and execution logic here.
    printf("Executing instruction at PC %d: %s\n", process->pc, instruction);

    // Increment the program counter to point to the next instruction
    process->pc += 1; // Adjust this based on your instruction size
}
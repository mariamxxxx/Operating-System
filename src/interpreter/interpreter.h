#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../process/processs.h"

char* substring(const char* src, int start, int length);
char** splitAndReverse(const char* str, int* count);
void callSemWait(Process *process , int resourceType);
void callSemSignal(int resourceType);
void callAssign(int pid , char* varName, char* varValue);
void callPrint(char* data);
void callPrintFromTo(int from, int to);
void callWriteFile(char* filename, char* content);
char* callReadFile(char* filename);
char* callTakeInput();
void execute_instruction(Process* process);

// True when the last execute_instruction bailed for GUI input (not yet submitted).
// Used to skip sim clock and RR/MLFQ quantum for that "poll".
void instruction_clear_stall(void);
int instruction_stalled_on_input(void);

#endif

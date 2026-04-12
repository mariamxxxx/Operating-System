#ifndef SYSCALLS_H
#define SYSCALLS_H

char* readFile(char* filename);
int writeFile(char* filename, char* content);
void printData(char* data);
char* takeInput();

char* readFromMemory(int pid, char* varName);
void writeToMemory(int pid, char* varName, char* varValue);
char* readInstruction(int pc);

#endif

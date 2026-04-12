#ifndef PARSER_H
#define PARSER_H

#include "../process/processs.h"

int CountLines(char* rawData);
void parseInstructionsIntoMemory(char* rawData , Process* process);
// extern void loadAndInterpret(char* filename);
extern void loadAndInterpret(char* filename, int arrival_time);



#endif
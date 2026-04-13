#include <stdio.h>  // For FILE, fopen, printf, fgets, fprintf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen
#include "syscalls.h"
#include "../memory/memoryy.h"

char* readFile(char* filename){ //read file
    FILE* f= fopen(filename,"r");
    if(f==NULL){
        printf("File not found\n");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* res= (char*)malloc(size+1); // DO WE CHANGE MALLOC BECAUSE OF OUR MADEUP MEMEORY
    fread(res, 1, size, f);
    res[size]='\0';
    fclose(f);
    return res;
}

int writeFile(char* filename, char* content){ //write in file
    FILE* f= fopen(filename,"w");
    if(f==NULL){
        printf("Could not open file for writing\n");
        return -1;
    }
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

void printData(char* data ){ //print values
    printf("Printed value: %s \n", data);
}

char* takeInput(){ //take input from user
    char* input = (char*) malloc(100);
    printf("Enter input: ");
    fgets(input, 100, stdin);
    int len = strlen(input); //here we remove \n if present
    if (len>0 && input[len-1] =='\n')
        input[len-1] ='\0';
    return input;
}

char* readFromMemory(int pid, char* varName){//read data from memory
//ASSUMING READ_WORD WILL BE IMPLEMENTED IN MEMORY.C
    char* res= read_word(pid, varName);

    if (res==NULL){
        printf("Variable %s not found in memory for process id %d\n", varName, pid);
        return NULL;}

    printf("Variable %s found in memory for process id %d with value: %s\n", varName,pid, res);
    return res;

}

void writeToMemory(int pid, char* varName, char* varValue){ //write data to memory
//ASSUMING WRITE__WORD WILL BE IMPLEMENTED IN MEMORY.C
    write_word(pid, varName, varValue);
    printf("Variable %s with value %s written to memory for process id %d\n", varName, varValue, pid);
}

char* readInstruction(int pc){
    return read_code_line(pc);
}

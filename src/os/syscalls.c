#include <stdio.h>  // For FILE, fopen, printf, fgets, fprintf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen
#include "syscalls.h"
#include "../memory/memoryy.h"

// Link to the GUI logger and GUI input buffer
extern void gui_log(const char* format, ...);
extern char input_text[];

static void build_disk_path(const char *name, char *out, size_t out_size) {
    if (name == NULL || out == NULL || out_size == 0) {
        return;
    }

    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_size, "%s", name);
        return;
    }

    snprintf(out, out_size, "src/disk/%s", name);
}

char* readFile(char* filename){ //read file
    char path[512];
    build_disk_path(filename, path, sizeof(path));

    FILE* f= fopen(path,"r");
    if(f==NULL){
        gui_log("File not found");
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
    char path[512];
    build_disk_path(filename, path, sizeof(path));

    FILE* f= fopen(path,"w");
    if(f==NULL){
        gui_log("ERROR: could not open file for writing");
        return -1;
    }
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

void printData(char* data ){ //print values
    gui_log("Printed value: %s ", data);
}

// [Keep everything else identical]

char* takeInput(){ 
    char* input = (char*) malloc(100);
    gui_log("Enter input: ");
    
    // Copy the guaranteed ready text
    strncpy(input, input_text, 99);
    input[99] = '\0';
    
    // Wipe the GUI text box clean now that it's consumed
    input_text[0] = '\0'; 

    int len = strlen(input); 
    if (len>0 && input[len-1] =='\n')
        input[len-1] ='\0';
    return input;
}

// [Keep everything else identical]

// char* takeInput(){ //take input from user
//     char* input = (char*) malloc(100);
//     gui_log("Enter input: "); // Exact print statement preserved
    
//     // Instead of freezing the GUI with fgets(..., stdin), 
//     // we copy whatever you typed into the GUI's input box!
//     strncpy(input, input_text, 99);
//     input[99] = '\0';
    
//     // Clear the GUI input buffer after reading it so we don't read it twice
//     input_text[0] = '\0';

//     int len = strlen(input); //here we remove \n if present
//     if (len>0 && input[len-1] =='\n')
//         input[len-1] ='\0';
//     return input;
// }

char* readFromMemory(int pid, char* varName){//read data from memory
//ASSUMING READ_WORD WILL BE IMPLEMENTED IN MEMORY.C
    char* res= read_word(pid, varName);

    if (res==NULL){
        gui_log("Variable %s not found in memory for process id %d", varName, pid);
        return NULL;}

    gui_log("Variable %s found in memory for process id %d with value: %s", varName,pid, res);
    return res;

}

void writeToMemory(int pid, char* varName, char* varValue){ //write data to memory
//ASSUMING WRITE__WORD WILL BE IMPLEMENTED IN MEMORY.C
    write_word(pid, varName, varValue);
    gui_log("Variable %s with value %s written to memory for process id %d", varName, varValue, pid);
}

char* readInstruction(int pc){
    return read_code_line(pc);
}
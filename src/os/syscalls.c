#include <stdio.h>  // For FILE, fopen, printf, fgets, fprintf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "syscalls.h"
#include "../memory/memoryy.h"
#include "../gui/gui.h"

static void ensure_disk_dir_exists(void) {
#if defined(_WIN32)
    _mkdir("src/disk");
#else
    mkdir("src/disk", 0777);
#endif
}

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
    ensure_disk_dir_exists();

    char path[512];
    build_disk_path(filename, path, sizeof(path));

    FILE* f= fopen(path,"rb");
    if(f==NULL){
        printf("File not found\n");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char* res= (char*)malloc((size_t)size + 1); // DO WE CHANGE MALLOC BECAUSE OF OUR MADEUP MEMEORY
    if (res == NULL) {
        fclose(f);
        return NULL;
    }
    size_t bytes_read = fread(res, 1, (size_t)size, f);
    res[bytes_read]='\0';
    fclose(f);
    return res;
}

int writeFile(char* filename, char* content){ //write in file
    ensure_disk_dir_exists();

    char path[512];
    build_disk_path(filename, path, sizeof(path));

    FILE* f= fopen(path,"w");
    if(f==NULL){
        printf("ERROR: could not open file for writing\n");
        return -1;
    }
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

void printData(char* data ){ //print values
    printf("Printed value: %s \n", data);
    gui_log("Printed value: %s \n", data);
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
        gui_log("Variable %s not found in memory for process id %d\n", varName, pid);
        return NULL;}

    printf("Variable %s found in memory for process id %d with value: %s\n", varName,pid, res);
    gui_log("Variable %s found in memory for process id %d with value: %s\n", varName,pid, res);
    return res;

}

void writeToMemory(int pid, char* varName, char* varValue){ //write data to memory
//ASSUMING WRITE__WORD WILL BE IMPLEMENTED IN MEMORY.C
    write_word(pid, varName, varValue);
    printf("Variable %s with value %s written to memory for process id %d\n", varName, varValue, pid);
    gui_log("Variable %s with value %s written to memory for process id %d\n", varName, varValue, pid);
}

char* readInstruction(int pc){
    return read_code_line(pc);
}

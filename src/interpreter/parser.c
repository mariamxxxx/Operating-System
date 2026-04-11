#include <stdio.h>  // For FILE, fopen, printf, fgets, fprintf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen



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

void parseInstructionsIntoMemory(char* rawData) {
    

    char* line = strtok(rawData, "\n"); // Get first line
    
    while (line != NULL) {

        char* instruction = parseLineToOpcode(line); 

        writetoMemory(instruction);

        line = strtok(NULL, "\n"); // Get next line
    }

    free(rawData); 
}

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
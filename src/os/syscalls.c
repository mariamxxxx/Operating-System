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
    printf("Printed value: %s", data);
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

char* readFromMemory(){//read data from memory
//where do i read from? what index do i use to find what i need to read?
// do i take any input? how do i know what i want to read?
}

void writetoMemory(char* value){ //write data to memory
//where do i write to? what index do i use to find where i need to write?
// do i take any input? where do i store it?
}
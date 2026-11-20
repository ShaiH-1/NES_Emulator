#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

/*
Function initialize memory
input: pointer to memory
output: None
*/
void initMemory(memory mem)
{
    for (int i = 0;i<MEMORY_SIZE;i++) //Clears memory
    {
        mem.RAM[i] = 0;
        mem.ROM[i] = 0;
    }
}


/*
Function loads ROM file into memory
input: Memory and path to ROM file
output: None
*/
void loadRom(char *path,memory mem)
{
    FILE *file = fopen(path,"r");

    if (!file)
    {
        printf("ERROR: File couldnt be opened");
        return;
    }

    fseek(file,0,SEEK_END);
    long romSize = ftell(file);
    rewind(file);

    if (romSize > MEMORY_SIZE)
    {
        printf("ERROR: ROM file bigger then memory");
        return;
    }

    char *romBuffer = (char*)malloc(sizeof(char) * romSize);

    size_t byteRead = fread(romBuffer,sizeof(uint8_t),romSize,file);

    for (int i = 0;i<romSize;i++)
    {
        mem.ROM[i] = (uint8_t)romBuffer[i]; 
    }

    free(romBuffer);
    fclose(file);
}

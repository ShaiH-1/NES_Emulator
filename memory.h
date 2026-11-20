#ifndef MEMORYH
#define MEMORYH

#include <stdint.h>

#define MEMORY_SIZE 2048
#define MEMORY_MASK (MEMORY_SIZE - 1)

typedef struct memory
{
    uint8_t ROM[MEMORY_SIZE];
    uint8_t RAM[MEMORY_SIZE];
}memory;

void initMemory(memory mem);
void loadRom(char *path,memory mem);

#endif
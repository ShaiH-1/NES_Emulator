#ifndef CPUH
#define CPUH

#include <stdint.h>
#include "memory.h"

#define CF (1 << 0) //Carry flag
#define ZF (1 << 1) //Zero flag
#define INF (1 << 2) //Interrupt Disable flag
#define DF (1 << 3) //Decimal flag
#define BF (1 << 4) //Break Flag
#define UF (1 << 5) //Unused flag (always on)
#define VF (1 << 6) //Overflow flag
#define NF (1 << 7) //Negative flag

typedef struct cpu
{
    memory memory; //Memory
    uint8_t A; //A register
    uint8_t IX; //Index X register
    uint8_t IY; //Index Y register
    uint16_t pc; //Program counter
    uint8_t sp; //stack pointer
    uint8_t P; //status register
    uint8_t opcode; //opcode
}cpu;

void initCpu(cpu *cpu);
void emluateCycle(cpu *cpu);

#endif
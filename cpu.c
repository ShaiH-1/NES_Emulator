#include "cpu.h"

/*
Function initialize cpu
input: pointer to the cpu
output: None
*/
void initCpu(cpu *cpu)
{
    cpu->A = 0;
    cpu->IX = 0;
    cpu->IY = 0;
    cpu->P = 0x24;
    cpu->pc = 0;
    cpu->sp = 0xFD;

    initMemory(cpu->memory);
}

/*
Helper: reads one byte from the instruction stream (ROM) and advances pc
input: pointer to cpu
output: the byte
*/
static uint8_t fetch8(cpu *cpu)
{
    return cpu->memory.ROM[(cpu->pc++) & MEMORY_MASK];
}

/*
reads a little endian 16 bit value from the instruction stream
input: pointer to cpu
output: the value
*/
static uint16_t fetch16(cpu *cpu)
{
    uint8_t lowByte = fetch8(cpu);
    uint8_t highByte = fetch8(cpu);

    return ((uint16_t)highByte << 8) | lowByte;
}

/*
Addressing modes - each one consumes its operand bytes and returns the
effective RAM address (already wrapped to the size of the RAM)
*/
static uint16_t addrZeroPage(cpu *cpu)
{
    return fetch8(cpu);
}

static uint16_t addrZeroPageX(cpu *cpu)
{
    return (uint8_t)(fetch8(cpu) + cpu->IX); //wraps inside the zero page
}

static uint16_t addrZeroPageY(cpu *cpu)
{
    return (uint8_t)(fetch8(cpu) + cpu->IY);
}

static uint16_t addrAbsolute(cpu *cpu)
{
    return fetch16(cpu) & MEMORY_MASK;
}

static uint16_t addrAbsoluteX(cpu *cpu)
{
    return (fetch16(cpu) + cpu->IX) & MEMORY_MASK;
}

static uint16_t addrAbsoluteY(cpu *cpu)
{
    return (fetch16(cpu) + cpu->IY) & MEMORY_MASK;
}

static uint16_t addrIndirectX(cpu *cpu) // (zp,X)
{
    uint8_t ptr = (uint8_t)(fetch8(cpu) + cpu->IX);
    uint8_t lowByte = cpu->memory.RAM[ptr];
    uint8_t highByte = cpu->memory.RAM[(uint8_t)(ptr + 1)];

    return (((uint16_t)highByte << 8) | lowByte) & MEMORY_MASK;
}

static uint16_t addrIndirectY(cpu *cpu) // (zp),Y
{
    uint8_t ptr = fetch8(cpu);
    uint8_t lowByte = cpu->memory.RAM[ptr];
    uint8_t highByte = cpu->memory.RAM[(uint8_t)(ptr + 1)];

    return ((((uint16_t)highByte << 8) | lowByte) + cpu->IY) & MEMORY_MASK;
}


/*
Function does pull from stack
input: pointer to cpu
output: uint8_t value from stack
*/
uint8_t popStack(cpu *cpu)
{
    cpu->sp++;
    return cpu->memory.RAM[0x0100 + cpu->sp];
}


/*
Function does push to stack
input: pointer to cpu and value to add
output: None
*/
void pushStack(cpu *cpu,uint8_t val)
{
    cpu->memory.RAM[cpu->sp + 0x0100] = val;
    cpu->sp--;
}


/*
Function does ADC 
input: pointer to CPU and value from memory
output: None
*/
void adc(cpu *cpu,uint8_t val)
{
    uint16_t res = cpu->A + val + (cpu->P & CF);

    if (res > 0xFF)
    {
        cpu->P |= CF;
    }
    else
    {
        cpu->P &= ~CF;
    }

    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if ((res ^ cpu->A) & (res ^ val) & 0x80)
    {
        cpu->P |= VF;
    }
    else
    {
        cpu->P &= ~VF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = (uint8_t)res;
}

/*
Function does AND
input: pointer to cpu and value from memory
output: None
*/
void and(cpu *cpu,uint8_t val)
{
    uint8_t res = cpu->A & val;

    if (res == 0x00)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = res;
}

/*
Function does ASL
input: pointer to cpu 
output: None
*/
void asl(cpu *cpu, uint8_t val)
{
    uint8_t res = val << 1;

    if (val & 0x80)
        cpu->P |= CF;
    else
        cpu->P &= ~CF;

    if (res == 0)
        cpu->P |= ZF;
    else
        cpu->P &= ~ZF;

    if (res & 0x80)
        cpu->P |= NF;
    else
        cpu->P &= ~NF;

    cpu->A = res;
}

/*
Function does BCC
input: pointer to cpu 
output: None
*/
void bcc(cpu *cpu,uint8_t val)
{   
    if ((cpu->P & CF) == 0)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BCS
input: pointer to cpu
output: None
*/
void bcs(cpu *cpu,uint8_t val)
{
    if (cpu->P & CF)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BEQ
input: pointer to cpu
output: None
*/
void beq(cpu *cpu,uint8_t val)
{
    if (cpu->P & ZF)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BIT 
input: pointer to cpu
output: None
*/
void bit(cpu *cpu, uint16_t addr)
{
    uint8_t val = cpu->memory.RAM[addr];

    if ((cpu->A & val) == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (val & 0x40)
    {
        cpu->P |= VF;
    }
    else
    {    
        cpu->P &= ~VF;
    }

    if (val & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does BMI
input: pointer to cpu
output: None
*/
void bmi(cpu *cpu,uint8_t val)
{
    if (cpu->P & NF)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BNE
input: pointer to cpu
output: None
*/
void bne(cpu *cpu,uint8_t val)
{
    if ((cpu->P & ZF) == 0)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BPL
input: pointer to cpu 
output: None
*/
void bpl(cpu *cpu,uint8_t val)
{
    if ((cpu->P & NF) == 0)
    {
        cpu->pc += 2 + val;
    }
}

/*
Function does BRK 
input: pointer to cpu
output: None
*/
void brk(cpu *cpu)
{

}

/*
Function does BVS
input: pointer to cpu
output: None
*/
void bvs(cpu *cpu)
{
    if (cpu->P & VF)
    {
        cpu->pc += 2 + cpu->memory.RAM[cpu->pc];
    }
}

/*
Function does CLC
input: pointer to cpu
output: None
*/
void clc(cpu *cpu)
{
    cpu->P &= ~CF;
}

/*
Function does CLD
input: pointer to cpu
output: None
*/
void cld(cpu *cpu)
{
    cpu->P &= ~DF;
}

/*
Function does CLI
input: pointer to cpu
output: None
*/
void cli(cpu *cpu)
{
    cpu->P &= ~INF;
}

/*
Function does CLV
input: pointer to cpu 
output: None
*/
void clv(cpu *cpu)
{
    cpu->P &= ~VF;
}

/*
Function does CMP
input: pointer to cpu and value
output: None
*/
void cmp(cpu *cpu,uint8_t val)
{
    if (cpu->A == val)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->A >= val)
    {
        cpu->P |= CF;
    }
    else
    {
        cpu->P &= ~CF;
    }

    if ((cpu->A - val) & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does DEC
input: pointer to cpu and adress to memory to decresse from
output: None
*/
void dec(cpu *cpu,uint16_t addr)
{
    cpu->memory.RAM[addr]--;
    if (cpu->memory.RAM[addr] == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->memory.RAM[addr] & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

}

/*
Function does dex
input: pointer to cpu
output: None
*/
void dex(cpu *cpu)
{
    cpu->IX--;
    if (cpu->IX == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IX & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does DEY
input: pointer to cpu
output: None
*/
void dey(cpu *cpu)
{
    cpu->IY--;

    if (cpu->IY == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IY & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does EOR
input: pointer to cpu and value
output: None
*/
void eor(cpu *cpu,uint8_t val)
{
    uint8_t res = cpu->A ^ val;

    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
    cpu->A = res;
}

/*
Function does INC
input: pointer to cpu and addr
output: None
*/
void inc(cpu *cpu,uint16_t addr)
{
    cpu->memory.RAM[addr]++;

    if (cpu->memory.RAM[addr] == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->memory.RAM[addr] & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does INX
input: pointer to cpu
output: None
*/
void inx(cpu *cpu)
{
    cpu->IX++;
    if (cpu->IX == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IX & 0x80)
    {
        cpu->P |= NF; 
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does INY
input: pointer to cpu
output: None
*/
void iny(cpu *cpu)
{
    cpu->IY++;

    if (cpu->IY == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IY & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does JMP
input: pointer to cpu and memory address
output: None
*/
void jmp(cpu *cpu,uint16_t addr)
{
    cpu->pc = addr;
}

/*
Function does JSR
input: pointer to cpu
output: None
*/
void jsr(cpu *cpu)
{
    uint8_t lowByte = cpu->memory.RAM[cpu->pc++];
    uint8_t highByte = cpu->memory.RAM[cpu->pc++];

    uint16_t targetAddr = ((highByte << 8) | lowByte);
    uint16_t returnAddr = cpu->pc - 1;
    pushStack(cpu,(returnAddr >> 8) & 0xFF); 
    pushStack(cpu,returnAddr & 0xFF);

    cpu->pc = targetAddr;
}

/*
Function does LDA
input: pointer to cpu and address to memory
output: None
*/
void lda(cpu *cpu,uint16_t addr)
{
    cpu->A = cpu->memory.RAM[addr];

    if (cpu->A == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->A & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does LDX
input: pointer to cpu and addres to memory
output: None
*/
void ldx(cpu *cpu,uint16_t addr)
{
    cpu->IX = cpu->memory.RAM[addr];
    
    if (cpu->IX == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IX & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does LDY
input: pointer to cpu and addres to memory
output: None
*/
void ldy(cpu *cpu,uint16_t addr)
{
    cpu->IY = cpu->memory.RAM[addr];

    if (cpu->IY == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->IY & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does NOP (just nothing)
input: pointer to cpu
output: None
*/
void nop(cpu *cpu)
{
    
}

/*
Function does ORA
input: pointer to cpu and value
output: None
*/
void ora(cpu *cpu,uint8_t value)
{
    uint8_t res = cpu->A | value;
    
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = res;
}

/*
Function does PHA
input: pointer to cpu
output: None
*/
void pha(cpu *cpu)
{
    pushStack(cpu,cpu->A);
}

/*
Function does PHP
input: pointer to cpu 
output: None
*/
void php(cpu *cpu)
{
    pushStack(cpu,cpu->P);
}

/*
Function does PLA
input: pointer to cpu
output: None
*/
void pla(cpu *cpu)
{
    cpu->A = popStack(cpu);

    if (cpu->A == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (cpu->A & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does PLP
input: pointer to cpu
output: None
*/
void plp(cpu *cpu)
{
    cpu->P = popStack(cpu);
}


/*
Function does ROL
input: pointer to cpu and address to memory
output: None
*/
void rol(cpu *cpu, uint16_t addr)
{
    uint8_t val = cpu->memory.RAM[addr];
    uint8_t oldCarry = (cpu->P & CF) ? 1 : 0;

    if (val & 0x80)
    {
        cpu->P |= CF;
    }
    else
    {
        cpu->P &= ~CF;
    }

    val = (val << 1) | oldCarry;
    cpu->memory.RAM[addr] = val;

    if (val == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (val & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}

/*
Function does ROR
input: pointer to cpu and address to memory
output: None
*/
void ror(cpu *cpu, uint16_t addr)
{
    uint8_t val = cpu->memory.RAM[addr];
    uint8_t oldCarry = (cpu->P & CF) ? 0x80 : 0;

    if (val & 0x01)
    {
        cpu->P |= CF;
    }
    else
    {
        cpu->P &= ~CF;
    }

    val = (val >> 1) | oldCarry;
    cpu->memory.RAM[addr] = val;

    if (val == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (val & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }
}


/*
Function does RTI
input: pointer to cpu
output: None
*/
void rti(cpu *cpu)
{
    cpu->P = popStack(cpu);

    uint8_t lowByte = popStack(cpu);
    uint8_t highByte = popStack(cpu);

    cpu->pc = ((uint16_t)highByte << 8) | lowByte;
}

/*
Function does RTS
input: pointer to cpu
output: None
*/
void rts(cpu *cpu)
{
    uint8_t lowByte = popStack(cpu);
    uint8_t highByte = popStack(cpu);

    cpu->pc = ((uint16_t)highByte << 8) | lowByte;
    cpu->pc++;
}


/*
Function does SBC
input: pointer to cpu and address to memory
output: None
*/
void sbc(cpu *cpu, uint16_t addr)
{
    uint8_t val = cpu->memory.RAM[addr];
    int borrow = (cpu->P & CF) ? 0 : 1;
    int res = (int)cpu->A - (int)val - borrow;

    if (res >= 0)
    {
        cpu->P |= CF;
    }
    else
    {
        cpu->P &= ~CF;
    }
        
    if ((uint8_t)res == 0)
    {
        cpu->P |= ZF;
    }
        
    else
    {
        cpu->P &= ~ZF;
    }
    
    if (((cpu->A ^ res) & 0x80) && ((cpu->A ^ val) & 0x80))
    {
        cpu->P |= VF;
    }

    else
    {
        cpu->P &= ~VF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;  
    } 
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = (uint8_t)res;
}

/*
Function does SEC
input: pointer to cpu 
output: None
*/
void sec(cpu *cpu)
{
    cpu->P |= CF;
}

/*
Function does SED
input: pointer to cpu
output: None
*/
void sed(cpu *cpu)
{
    cpu->P |= DF;
}

/*
Function does SEI
input: pointer to cpu
output: None
*/
void sei(cpu *cpu)
{
    cpu->P |= INF;
}

/*
Function does STA
input: pointer to cpu and address to memory
output: None
*/
void sta(cpu *cpu,uint16_t addr)
{
    cpu->memory.RAM[addr] = cpu->A;
}

/*
Function does STX
input: pointer to cpu and address to memory
output: None
*/
void stx(cpu *cpu,uint16_t addr)
{
    cpu->memory.RAM[addr] = cpu->IX;
}

/*
Function does STY
input: pointer to cpu and address to memory
output: None
*/
void sty(cpu *cpu,uint16_t addr)
{
    cpu->memory.RAM[addr] = cpu->IY;
}

/*
Function does TAX
input: pointer to cpu
output: None
*/
void tax(cpu *cpu)
{
    uint8_t res = cpu->A;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->IX = res;
}

/*
Function does TAY
input: pointer to cpu
output: None
*/
void tay(cpu *cpu)
{
    uint8_t res = cpu->A;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->IY = res;
}

/*
Function does TSX
input: pointer to cpu
output: None
*/
void tsx(cpu *cpu)
{
    uint8_t res = cpu->sp;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->IX = res;
}

/*
Function does TXA
input: pointer to cpu
output: None
*/
void txa(cpu *cpu)
{
    uint8_t res = cpu->IX;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = res;
}


/*
Function does TXS
input: pointer to cpu
output: None
*/
void txs(cpu *cpu)
{
    uint8_t res = cpu->IX;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->sp = res;
}

/*
Function does TYA
input: pointer to cpu
output: None
*/
void tya(cpu *cpu)
{
    uint8_t res = cpu->IY;
    if (res == 0)
    {
        cpu->P |= ZF;
    }
    else
    {
        cpu->P &= ~ZF;
    }

    if (res & 0x80)
    {
        cpu->P |= NF;
    }
    else
    {
        cpu->P &= ~NF;
    }

    cpu->A = res;
}




/*
Function emulate one cycle 
input: pointer to cpu 
output: None
*/
void emluateCycle(cpu *cpu)
{
    uint8_t opcode = fetch8(cpu);

    switch (opcode)
    {
    /* ---------------- ADC ---------------- */
    case 0x69: adc(cpu, fetch8(cpu));              break;
    case 0x65: adc(cpu, READ(addrZeroPage));       break;
    case 0x75: adc(cpu, READ(addrZeroPageX));      break;
    case 0x6D: adc(cpu, READ(addrAbsolute));       break;
    case 0x7D: adc(cpu, READ(addrAbsoluteX));      break;
    case 0x79: adc(cpu, READ(addrAbsoluteY));      break;
    case 0x61: adc(cpu, READ(addrIndirectX));      break;
    case 0x71: adc(cpu, READ(addrIndirectY));      break;

    /* ---------------- AND ---------------- */
    case 0x29: and(cpu, fetch8(cpu));              break;
    case 0x25: and(cpu, READ(addrZeroPage));       break;
    case 0x35: and(cpu, READ(addrZeroPageX));      break;
    case 0x2D: and(cpu, READ(addrAbsolute));       break;
    case 0x3D: and(cpu, READ(addrAbsoluteX));      break;
    case 0x39: and(cpu, READ(addrAbsoluteY));      break;
    case 0x21: and(cpu, READ(addrIndirectX));      break;
    case 0x31: and(cpu, READ(addrIndirectY));      break;

    /* ---------------- ASL ---------------- */
    case 0x0A: cpu->A = asl(cpu, cpu->A);          break;
    case 0x06: RMW(asl, addrZeroPage);             break;
    case 0x16: RMW(asl, addrZeroPageX);            break;
    case 0x0E: RMW(asl, addrAbsolute);             break;
    case 0x1E: RMW(asl, addrAbsoluteX);            break;

    /* ---------------- Branches ---------------- */
    case 0x90: bcc(cpu, fetch8(cpu));              break;
    case 0xB0: bcs(cpu, fetch8(cpu));              break;
    case 0xF0: beq(cpu, fetch8(cpu));              break;
    case 0x30: bmi(cpu, fetch8(cpu));              break;
    case 0xD0: bne(cpu, fetch8(cpu));              break;
    case 0x10: bpl(cpu, fetch8(cpu));              break;
    case 0x50: bvc(cpu, fetch8(cpu));              break;
    case 0x70: bvs(cpu, fetch8(cpu));              break;

    /* ---------------- BIT ---------------- */
    case 0x24: bit(cpu, addrZeroPage(cpu));        break;
    case 0x2C: bit(cpu, addrAbsolute(cpu));        break;

    /* ---------------- BRK ---------------- */
    case 0x00: brk(cpu);                           break;

    /* ---------------- Flag instructions ---------------- */
    case 0x18: clc(cpu);                           break;
    case 0xD8: cld(cpu);                           break;
    case 0x58: cli(cpu);                           break;
    case 0xB8: clv(cpu);                           break;
    case 0x38: sec(cpu);                           break;
    case 0xF8: sed(cpu);                           break;
    case 0x78: sei(cpu);                           break;

    /* ---------------- CMP ---------------- */
    case 0xC9: cmp(cpu, fetch8(cpu));              break;
    case 0xC5: cmp(cpu, READ(addrZeroPage));       break;
    case 0xD5: cmp(cpu, READ(addrZeroPageX));      break;
    case 0xCD: cmp(cpu, READ(addrAbsolute));       break;
    case 0xDD: cmp(cpu, READ(addrAbsoluteX));      break;
    case 0xD9: cmp(cpu, READ(addrAbsoluteY));      break;
    case 0xC1: cmp(cpu, READ(addrIndirectX));      break;
    case 0xD1: cmp(cpu, READ(addrIndirectY));      break;

    /* ---------------- CPX / CPY ---------------- */
    case 0xE0: cpx(cpu, fetch8(cpu));              break;
    case 0xE4: cpx(cpu, READ(addrZeroPage));       break;
    case 0xEC: cpx(cpu, READ(addrAbsolute));       break;
    case 0xC0: cpy(cpu, fetch8(cpu));              break;
    case 0xC4: cpy(cpu, READ(addrZeroPage));       break;
    case 0xCC: cpy(cpu, READ(addrAbsolute));       break;

    /* ---------------- DEC / INC ---------------- */
    case 0xC6: dec(cpu, addrZeroPage(cpu));        break;
    case 0xD6: dec(cpu, addrZeroPageX(cpu));       break;
    case 0xCE: dec(cpu, addrAbsolute(cpu));        break;
    case 0xDE: dec(cpu, addrAbsoluteX(cpu));       break;
    case 0xE6: inc(cpu, addrZeroPage(cpu));        break;
    case 0xF6: inc(cpu, addrZeroPageX(cpu));       break;
    case 0xEE: inc(cpu, addrAbsolute(cpu));        break;
    case 0xFE: inc(cpu, addrAbsoluteX(cpu));       break;

    /* ---------------- DEX / DEY / INX / INY ---------------- */
    case 0xCA: dex(cpu);                           break;
    case 0x88: dey(cpu);                           break;
    case 0xE8: inx(cpu);                           break;
    case 0xC8: iny(cpu);                           break;

    /* ---------------- EOR ---------------- */
    case 0x49: eor(cpu, fetch8(cpu));              break;
    case 0x45: eor(cpu, READ(addrZeroPage));       break;
    case 0x55: eor(cpu, READ(addrZeroPageX));      break;
    case 0x4D: eor(cpu, READ(addrAbsolute));       break;
    case 0x5D: eor(cpu, READ(addrAbsoluteX));      break;
    case 0x59: eor(cpu, READ(addrAbsoluteY));      break;
    case 0x41: eor(cpu, READ(addrIndirectX));      break;
    case 0x51: eor(cpu, READ(addrIndirectY));      break;

    /* ---------------- JMP / JSR ---------------- */
    case 0x4C: jmp(cpu, fetch16(cpu));             break;
    case 0x6C: //JMP (indirect), including the real 6502 page wrap bug
    {
        uint16_t ptr = fetch16(cpu);
        uint8_t lowByte = cpu->memory.RAM[ptr & MEMORY_MASK];
        uint8_t highByte = cpu->memory.RAM[((ptr & 0xFF00) | ((ptr + 1) & 0x00FF)) & MEMORY_MASK];

        jmp(cpu, ((uint16_t)highByte << 8) | lowByte);
        break;
    }
    case 0x20: jsr(cpu, fetch16(cpu));             break;

    /* ---------------- LDA ---------------- */
    case 0xA9: lda(cpu, fetch8(cpu));              break;
    case 0xA5: lda(cpu, READ(addrZeroPage));       break;
    case 0xB5: lda(cpu, READ(addrZeroPageX));      break;
    case 0xAD: lda(cpu, READ(addrAbsolute));       break;
    case 0xBD: lda(cpu, READ(addrAbsoluteX));      break;
    case 0xB9: lda(cpu, READ(addrAbsoluteY));      break;
    case 0xA1: lda(cpu, READ(addrIndirectX));      break;
    case 0xB1: lda(cpu, READ(addrIndirectY));      break;

    /* ---------------- LDX ---------------- */
    case 0xA2: ldx(cpu, fetch8(cpu));              break;
    case 0xA6: ldx(cpu, READ(addrZeroPage));       break;
    case 0xB6: ldx(cpu, READ(addrZeroPageY));      break;
    case 0xAE: ldx(cpu, READ(addrAbsolute));       break;
    case 0xBE: ldx(cpu, READ(addrAbsoluteY));      break;

    /* ---------------- LDY ---------------- */
    case 0xA0: ldy(cpu, fetch8(cpu));              break;
    case 0xA4: ldy(cpu, READ(addrZeroPage));       break;
    case 0xB4: ldy(cpu, READ(addrZeroPageX));      break;
    case 0xAC: ldy(cpu, READ(addrAbsolute));       break;
    case 0xBC: ldy(cpu, READ(addrAbsoluteX));      break;

    /* ---------------- LSR ---------------- */
    case 0x4A: cpu->A = lsr(cpu, cpu->A);          break;
    case 0x46: RMW(lsr, addrZeroPage);             break;
    case 0x56: RMW(lsr, addrZeroPageX);            break;
    case 0x4E: RMW(lsr, addrAbsolute);             break;
    case 0x5E: RMW(lsr, addrAbsoluteX);            break;

    /* ---------------- NOP ---------------- */
    case 0xEA: nop(cpu);                           break;

    /* ---------------- ORA ---------------- */
    case 0x09: ora(cpu, fetch8(cpu));              break;
    case 0x05: ora(cpu, READ(addrZeroPage));       break;
    case 0x15: ora(cpu, READ(addrZeroPageX));      break;
    case 0x0D: ora(cpu, READ(addrAbsolute));       break;
    case 0x1D: ora(cpu, READ(addrAbsoluteX));      break;
    case 0x19: ora(cpu, READ(addrAbsoluteY));      break;
    case 0x01: ora(cpu, READ(addrIndirectX));      break;
    case 0x11: ora(cpu, READ(addrIndirectY));      break;

    /* ---------------- Stack ---------------- */
    case 0x48: pha(cpu);                           break;
    case 0x08: php(cpu);                           break;
    case 0x68: pla(cpu);                           break;
    case 0x28: plp(cpu);                           break;

    /* ---------------- ROL ---------------- */
    case 0x2A: cpu->A = rol(cpu, cpu->A);          break;
    case 0x26: RMW(rol, addrZeroPage);             break;
    case 0x36: RMW(rol, addrZeroPageX);            break;
    case 0x2E: RMW(rol, addrAbsolute);             break;
    case 0x3E: RMW(rol, addrAbsoluteX);            break;

    /* ---------------- ROR ---------------- */
    case 0x6A: cpu->A = ror(cpu, cpu->A);          break;
    case 0x66: RMW(ror, addrZeroPage);             break;
    case 0x76: RMW(ror, addrZeroPageX);            break;
    case 0x6E: RMW(ror, addrAbsolute);             break;
    case 0x7E: RMW(ror, addrAbsoluteX);            break;

    /* ---------------- RTI / RTS ---------------- */
    case 0x40: rti(cpu);                           break;
    case 0x60: rts(cpu);                           break;

    /* ---------------- SBC ---------------- */
    case 0xE9: sbc(cpu, fetch8(cpu));              break;
    case 0xE5: sbc(cpu, READ(addrZeroPage));       break;
    case 0xF5: sbc(cpu, READ(addrZeroPageX));      break;
    case 0xED: sbc(cpu, READ(addrAbsolute));       break;
    case 0xFD: sbc(cpu, READ(addrAbsoluteX));      break;
    case 0xF9: sbc(cpu, READ(addrAbsoluteY));      break;
    case 0xE1: sbc(cpu, READ(addrIndirectX));      break;
    case 0xF1: sbc(cpu, READ(addrIndirectY));      break;

    /* ---------------- STA ---------------- */
    case 0x85: sta(cpu, addrZeroPage(cpu));        break;
    case 0x95: sta(cpu, addrZeroPageX(cpu));       break;
    case 0x8D: sta(cpu, addrAbsolute(cpu));        break;
    case 0x9D: sta(cpu, addrAbsoluteX(cpu));       break;
    case 0x99: sta(cpu, addrAbsoluteY(cpu));       break;
    case 0x81: sta(cpu, addrIndirectX(cpu));       break;
    case 0x91: sta(cpu, addrIndirectY(cpu));       break;

    /* ---------------- STX / STY ---------------- */
    case 0x86: stx(cpu, addrZeroPage(cpu));        break;
    case 0x96: stx(cpu, addrZeroPageY(cpu));       break;
    case 0x8E: stx(cpu, addrAbsolute(cpu));        break;
    case 0x84: sty(cpu, addrZeroPage(cpu));        break;
    case 0x94: sty(cpu, addrZeroPageX(cpu));       break;
    case 0x8C: sty(cpu, addrAbsolute(cpu));        break;

    /* ---------------- Transfers ---------------- */
    case 0xAA: tax(cpu);                           break;
    case 0xA8: tay(cpu);                           break;
    case 0xBA: tsx(cpu);                           break;
    case 0x8A: txa(cpu);                           break;
    case 0x9A: txs(cpu);                           break;
    case 0x98: tya(cpu);                           break;

    default:
        printf("Invalid opcode: 0x%02X at 0x%04X\n", opcode, (uint16_t)(cpu->pc - 1));
        break;
    }
}
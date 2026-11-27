#include "cpu.h"
#include <stdio.h>

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

    initMemory(&cpu->memory);
}


//Reads one byte from the instruction stream (ROM) and advances pc
static uint8_t fetch8(cpu *c)
{
    return c->memory.ROM[(c->pc++) & MEMORY_MASK];
}

//Reads a little endian 16 bit value from the instruction stream
static uint16_t fetch16(cpu *c)
{
    uint8_t lowByte = fetch8(c);
    uint8_t highByte = fetch8(c);

    return ((uint16_t)highByte << 8) | lowByte;
}


//Addressing modes: each one reads its operand bytes and returns the RAM address
static uint16_t addrZeroPage(cpu *c)
{
    return fetch8(c);
}

static uint16_t addrZeroPageX(cpu *c)
{
    return (uint8_t)(fetch8(c) + c->IX);
}

static uint16_t addrZeroPageY(cpu *c)
{
    return (uint8_t)(fetch8(c) + c->IY);
}

static uint16_t addrAbsolute(cpu *c)
{
    return fetch16(c) & MEMORY_MASK;
}

static uint16_t addrAbsoluteX(cpu *c)
{
    return (fetch16(c) + c->IX) & MEMORY_MASK;
}

static uint16_t addrAbsoluteY(cpu *c)
{
    return (fetch16(c) + c->IY) & MEMORY_MASK;
}

static uint16_t addrIndirectX(cpu *c) // (zp,X)
{
    uint8_t ptr = (uint8_t)(fetch8(c) + c->IX);
    uint8_t lowByte = c->memory.RAM[ptr];
    uint8_t highByte = c->memory.RAM[(uint8_t)(ptr + 1)];

    return (((uint16_t)highByte << 8) | lowByte) & MEMORY_MASK;
}

static uint16_t addrIndirectY(cpu *c) // (zp),Y
{
    uint8_t ptr = fetch8(c);
    uint8_t lowByte = c->memory.RAM[ptr];
    uint8_t highByte = c->memory.RAM[(uint8_t)(ptr + 1)];

    return ((((uint16_t)highByte << 8) | lowByte) + c->IY) & MEMORY_MASK;
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
    uint8_t res = cpu->A + val + (cpu->P & CF);

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
        cpu->pc += (uint8_t)val;
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
        cpu->pc += (uint8_t)val;
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
        cpu->pc += (uint8_t)val;
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
        cpu->pc += (uint8_t)val;
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
        cpu->pc += (uint8_t)val;
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
        cpu->pc += (uint8_t)val;
    }
}

/*
Function does BRK 
input: pointer to cpu
output: None
*/
void brk(cpu *cpu)
{
    pushStack(cpu, (cpu->pc >> 8) & 0xFF);
    pushStack(cpu, cpu->pc & 0xFF);

    pushStack(cpu ,cpu->P | 0x30);
    cpu->P |= INF;

    uint8_t lowByte = cpu->memory.ROM[0xFFFE];
    uint8_t highByte = cpu->memory.ROM[0xFFFF];

    cpu->pc = ((uint16_t)highByte <<8) | lowByte;
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
        cpu->pc += (uint8_t)cpu->memory.RAM[cpu->pc];
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
    pushStack(cpu,cpu->P | 0x30);
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
    cpu->P = (popStack(cpu) & ~0x10) | 0x20;
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
    cpu->sp = cpu->IX; 
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
    cpu->opcode = fetch8(cpu);

    uint8_t opcode = cpu->opcode;
    uint16_t addr;

    switch (opcode)
    {
        // ---------------- ADC ----------------

        case 0x69: // Immediate
            adc(cpu, fetch8(cpu));
            break;

        case 0x65: // Zero Page
            addr = addrZeroPage(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x75: // Zero Page,X
            addr = addrZeroPageX(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x6D: // Absolute
            addr = addrAbsolute(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x7D: // Absolute,X
            addr = addrAbsoluteX(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x79: // Absolute,Y
            addr = addrAbsoluteY(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x61: // Indirect,X
            addr = addrIndirectX(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x71: // Indirect,Y
            addr = addrIndirectY(cpu);
            adc(cpu, cpu->memory.RAM[addr]);
            break;


        // ---------------- AND ----------------

        case 0x29:
            and(cpu, fetch8(cpu));
            break;

        case 0x25:
            addr = addrZeroPage(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x35:
            addr = addrZeroPageX(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x2D:
            addr = addrAbsolute(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x3D:
            addr = addrAbsoluteX(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x39:
            addr = addrAbsoluteY(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x21:
            addr = addrIndirectX(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;

        case 0x31:
            addr = addrIndirectY(cpu);
            and(cpu, cpu->memory.RAM[addr]);
            break;


        // ---------------- LDA ----------------

        case 0xA9: // Immediate
            cpu->A = fetch8(cpu);

            if (cpu->A == 0)
                cpu->P |= ZF;
            else
                cpu->P &= ~ZF;

            if (cpu->A & 0x80)
                cpu->P |= NF;
            else
                cpu->P &= ~NF;
            break;

        case 0xA5:
            addr = addrZeroPage(cpu);
            lda(cpu, addr);
            break;

        case 0xB5:
            addr = addrZeroPageX(cpu);
            lda(cpu, addr);
            break;

        case 0xAD:
            addr = addrAbsolute(cpu);
            lda(cpu, addr);
            break;

        case 0xBD:
            addr = addrAbsoluteX(cpu);
            lda(cpu, addr);
            break;

        case 0xB9:
            addr = addrAbsoluteY(cpu);
            lda(cpu, addr);
            break;

        case 0xA1:
            addr = addrIndirectX(cpu);
            lda(cpu, addr);
            break;

        case 0xB1:
            addr = addrIndirectY(cpu);
            lda(cpu, addr);
            break;


        // ---------------- STA ----------------

        case 0x85:
            addr = addrZeroPage(cpu);
            sta(cpu, addr);
            break;

        case 0x95:
            addr = addrZeroPageX(cpu);
            sta(cpu, addr);
            break;

        case 0x8D:
            addr = addrAbsolute(cpu);
            sta(cpu, addr);
            break;

        case 0x9D:
            addr = addrAbsoluteX(cpu);
            sta(cpu, addr);
            break;

        case 0x99:
            addr = addrAbsoluteY(cpu);
            sta(cpu, addr);
            break;

        case 0x81:
            addr = addrIndirectX(cpu);
            sta(cpu, addr);
            break;

        case 0x91:
            addr = addrIndirectY(cpu);
            sta(cpu, addr);
            break;


        // ---------------- FLAGS ----------------

        case 0x18:
            clc(cpu);
            break;

        case 0x38:
            sec(cpu);
            break;

        case 0x58:
            cli(cpu);
            break;

        case 0x78:
            sei(cpu);
            break;

        case 0xB8:
            clv(cpu);
            break;

        case 0xD8:
            cld(cpu);
            break;

        case 0xF8:
            sed(cpu);
            break;


        // ---------------- TRANSFERS ----------------

        case 0xAA:
            tax(cpu);
            break;

        case 0xA8:
            tay(cpu);
            break;

        case 0x8A:
            txa(cpu);
            break;

        case 0x98:
            tya(cpu);
            break;

        case 0xBA:
            tsx(cpu);
            break;

        case 0x9A:
            txs(cpu);
            break;


        // ---------------- INCREMENT / DECREMENT ----------------

        case 0xE8:
            inx(cpu);
            break;

        case 0xC8:
            iny(cpu);
            break;

        case 0xCA:
            dex(cpu);
            break;

        case 0x88:
            dey(cpu);
            break;


        // ---------------- STACK ----------------

        case 0x48:
            pha(cpu);
            break;

        case 0x08:
            php(cpu);
            break;

        case 0x68:
            pla(cpu);
            break;

        case 0x28:
            plp(cpu);
            break;


        // ---------------- JUMPS / SUBROUTINES ----------------

        case 0x4C:
            jmp(cpu, fetch16(cpu));
            break;

        case 0x20:
            jsr(cpu);
            break;

        case 0x60:
            rts(cpu);
            break;

        case 0x40:
            rti(cpu);
            break;


        // ---------------- BRANCHES ----------------

        case 0x90:
            bcc(cpu, fetch8(cpu));
            break;

        case 0xB0:
            bcs(cpu, fetch8(cpu));
            break;

        case 0xF0:
            beq(cpu, fetch8(cpu));
            break;

        case 0x30:
            bmi(cpu, fetch8(cpu));
            break;

        case 0xD0:
            bne(cpu, fetch8(cpu));
            break;

        case 0x10:
            bpl(cpu, fetch8(cpu));
            break;


        // ---------------- OTHER ----------------

        case 0x00:
            brk(cpu);
            break;

        case 0xEA:
            nop(cpu);
            break;


        default:
            printf("ERROR: Unrecognized opcode: 0x%02X\n", opcode);
            break;
    }
}
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
Function emulate one cycle 
input: pointer to cpu 
output: None
*/
void emluateCycle(cpu *cpu)
{
    uint8_t opcode = fetch8(cpu);


    switch (opcode & 0xF0)
    {
    case 0x10:
        switch (opcode & 0x0F)
        {
            
        }
        break;
    
    case 0x20:
        switch (opcode & 0x0F)
        {
            case 0x05:
                and(cpu,cpu->memory.RAM[val]);
                cpu->pc += 2;
            case 0x09:
                and(cpu,val);
                cpu->pc += 2;
            
            default:
            break;
        }
        break;
    
    case 0x60:
        switch (opcode & 0x0F)
        {
        case 0x01:
            adc(cpu,cpu->memory.RAM[cpu->IX]);
            break;

        case 0x05:
            adc(cpu, cpu->memory.RAM[val]);
            break;

        case 0x09:
            adc(cpu,val);
            break;

        case 0x0D:
            adc(cpu,val);
            break;
        default:
            printf("Invalid opcode: 0x%X",opcode);
        }
    
    case 0x70:
        switch (opcode & 0x0F)
        {
        case 0x01:
            adc(cpu,cpu->memory.RAM[cpu->IY]);
            break;
        
        case 0x05:
            break;
        default:
            break;
        }
    
    break;
    
    
    
    default:
        break;
    }

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
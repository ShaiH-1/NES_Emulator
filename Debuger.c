#include <stdio.h>
#include "Debuger.h"
/*
Function prints new line out of '-' char 
input: None
output: None
*/
void printNL()
{
	for (int i = 0;i<LINE_LEN;i++)
	{
		printf("-");
	}
	printf("\n");
}

/*
Function debug cpu
input: pointer to cpu
output: None
*/
void debuger(cpu *sys)
{
	printf("opcode\t|register\t|program counter\t\n");
	while (sys->opcode != 0xFF) //Running until shutdown
	{
		emluateCycle(sys);

		printNL();
		printf("0x%x\t|A->0x%x\t\t|pc->0x%x\t\n",sys->opcode,sys->A,sys->pc);
	}
}



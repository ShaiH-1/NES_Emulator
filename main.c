#include "Debuger.h"
#include "memory.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 2) //In case there is not enough argoments 
	{
		printf("ERROR: Not enough parameters\n");
		printf("Example: emu.exe <path to ROM>\n");
		printf("Press any key to continue\n");
		getchar();
		return -1;
	}

	cpu sys;
	initCpu(&sys);
	loadRom(argv[1] ,&sys.memory);
    debuger(&sys);
}
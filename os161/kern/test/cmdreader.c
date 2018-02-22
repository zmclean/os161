#include <types.h>
#include <lib.h>
#include <test.h>

int printLine(int argc, char **argv)
{
	for(int i = 1; i < argc; i++)
	{
		kprintf("%s ", argv[i]);
	}
	kprintf("\n");
	return 0;
}


#include "gba.h"



int main()
{
	GBA gba;

	int x = 0;
	while (x<100)
	{
		gba.tick();
		x += 1;
	}
}
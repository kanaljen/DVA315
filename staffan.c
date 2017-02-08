#include <stdio.h>
#include "staffan.h"

void flushInput()
{
	char c;
	while ((c = getchar()) != '\n' && c != EOF) {}
}

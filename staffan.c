#include <stdio.h>
#include <windows.h>
#include "staffan.h"

void flushInput()
{
	char c;
	while ((c = getchar()) != '\n' && c != EOF) {}
}
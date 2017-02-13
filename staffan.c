#include <stdio.h>
#include <windows.h>
#include "staffan.h"

void flushInput()
{
	char c;
	while ((c = getchar()) != '\n' && c != EOF) {}
}

HANDLE connectToServerMailslot()
{
	HANDLE serverMailSlot = mailslotConnect("serverMailSlot");
	if (serverMailSlot == INVALID_HANDLE_VALUE) {
		printf("Failed to connect to the server-mailslot!\n");
		return serverMailSlot;
	}
}

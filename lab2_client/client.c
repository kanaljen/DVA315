/*********************************************
* client.c
*
* Desc: lab-skeleton for the client side of an
* client-server application
* 
* Revised by Dag Nystrom & Jukka Maki-Turja
* NOTE: the server must be started BEFORE the
* client.
*********************************************/
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "../wrapper.h"

#define MESSAGE "Hello!"

void main(void) {
	getchar();
	HANDLE mailSlot;
	DWORD bytesWritten;
	int loops = 2000;

	mailSlot = mailslotConnect("serverbox"); 

	if (mailSlot == INVALID_HANDLE_VALUE) {
		printf("Failed to get a handle to the mailslot!!\nHave you started the server?\n");
		getchar();
		return;
	}

						/* NOTE: replace code below for sending planet data to the server. */
	while(loops-- > 0) {
						/* send a friendly greeting to the server */
					/* NOTE: The messages sent to the server need not to be of equal size.       */
					/* Messages can be of different sizes as long as they don't exceed the       */
					/* maximum message size that the mailslot can handle (defined upon creation).*/
    
		bytesWritten = mailslotWrite (mailSlot, MESSAGE, strlen(MESSAGE));
		if (bytesWritten!=-1)
			printf("data sent to server (bytes = %d)\n", bytesWritten);
		else
			printf("failed sending data to server\n");
	}

	mailslotClose (mailSlot);

					/* (sleep for a while, enables you to catch a glimpse of what the */
					/*  client prints on the console)                                 */
	Sleep(2000);
	getchar();
	return;
}

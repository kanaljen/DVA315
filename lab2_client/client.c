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
#include "../staffan.h"

#define MESSAGE "Hello!"

DWORD WINAPI mailThread();

void main(void) {

	HANDLE startup = CreateEvent(NULL,0,0,"startup");

	DWORD mailThreadID = threadCreate(mailThread,NULL);
	HANDLE mailThread = OpenThread(THREAD_ALL_ACCESS,0,mailThreadID);
	WaitForSingleObject(startup, INFINITE);

	HANDLE serverMailSlot = mailslotConnect("server");
	if (serverMailSlot == INVALID_HANDLE_VALUE) {
		printf("Failed to connect to the server-mailslot!\n");
		getchar();
		return;
	}

	

	//INPUT LOOP
	DWORD bytesWritten;
	char c;
	do {
		bytesWritten = mailslotWrite(serverMailSlot, MESSAGE, strlen(MESSAGE));
		if (bytesWritten != -1)printf("data sent to server (bytes = %d)\n", bytesWritten);
		else printf("failed sending data to server\n");
		printf("Press any key to create another planet, press 'n' to quit!\n");
		c = getchar();
		flushInput();
	} while (c != 'n');
	//END LOOP

	TerminateThread(mailThread,NULL); //Send kill signal to mail-thread
	WaitForSingleObject(mailThread, INFINITE); //Wait for mail-thread to close
	mailslotClose (serverMailSlot); //close connection to mailslot
	return;
}

DWORD WINAPI mailThread()
{
	char processID[10];
	sprintf_s(processID, 10, "%d", GetCurrentProcessId()); //Convert ProcessID int to char
	HANDLE mailSlot = mailslotCreate(processID);
	if (mailSlot == INVALID_HANDLE_VALUE) {
		printf("Failed to get create client mailslot!\n");
		getchar();
		return;
	}
	//END Start client-mailslot

	int msgSize;
	char msg[1024];
	int bytesRead;
	while (TRUE) {
		GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0); //RETRIVE SIZE OF MSG
		if (msgSize > 0) {
			bytesRead = mailslotRead(mailSlot, msg, msgSize);
			printf("%s",msg);
		}
		Sleep(200);
	};
	return 0;
}


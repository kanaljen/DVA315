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


DWORD WINAPI mailThread();

void main(void) {

	HANDLE startup = CreateEvent(NULL,0,0,"startup");

	DWORD mailThreadID = threadCreate(mailThread,NULL);
	HANDLE mailThread = OpenThread(THREAD_ALL_ACCESS,0,mailThreadID);
	WaitForSingleObject(startup, INFINITE);

	HANDLE serverMailSlot = connectToServerMailslot();

	//INPUT LOOP
	DWORD bytesWritten;
	planet_type* newPlanet = (planet_type*)malloc(sizeof(planet_type));
	do {
		printf("\n[INPUT] Name of new planet: ");
		gets_s(newPlanet->name, 20);
		newPlanet->life = 200000000;
		newPlanet->mass = 1000;
		newPlanet->next = NULL;
		sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());
		newPlanet->sx = 200;
		newPlanet->sy = 300;
		newPlanet->vx = 0;
		newPlanet->vy = 0.008;
		bytesWritten = mailslotWrite(serverMailSlot, newPlanet, sizeof(planet_type));
		if (bytesWritten != -1)printf("\nData sent to server (bytes = %d)\n", bytesWritten);
		else printf("\nFailed sending data to server\n");
		Sleep(500);
		printf("\n[INPUT] Name of new planet: ");
		gets_s(newPlanet->name, 20);
		newPlanet->life = 200000000;
		newPlanet->mass = 100000000;
		newPlanet->next = NULL;
		sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());
		newPlanet->sx = 300;
		newPlanet->sy = 300;
		newPlanet->vx = 0;
		newPlanet->vy = 0;
		bytesWritten = mailslotWrite(serverMailSlot, newPlanet, sizeof(planet_type));
		if (bytesWritten != -1)printf("\nData sent to server (bytes = %d)\n", bytesWritten);
		else printf("\nFailed sending data to server\n");
		Sleep(500);
	} while (TRUE);
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
		printf("\nFailed to get create client mailslot!\n");
		getchar();
		return;
	}
	//END Start client-mailslot

	int msgSize;
	char msg[1024];
	while (TRUE) {
		GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0); //RETRIVE SIZE OF MSG
		if (msgSize > 0) {
			mailslotRead(mailSlot, msg, msgSize);
			msg[msgSize] = '\0';
			printf("\n%s",msg);
		}
		Sleep(200);
	};
	return 0;
}


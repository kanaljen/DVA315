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

	HANDLE startup = CreateEvent(NULL,1,0,"startup");

	DWORD mailThreadID = threadCreate(mailThread,NULL);
	HANDLE mailThread = OpenThread(THREAD_ALL_ACCESS,0,mailThreadID);
	WaitForSingleObject(startup, INFINITE);

	HANDLE serverMailSlot = connectToServerMailslot();

	//INPUT LOOP
	DWORD bytesWritten;
	time_t t;
	srand((unsigned)time(&t));
	int sys, aut, neg, numb;
	
	do {
		planet_type* newPlanet = (planet_type*)malloc(sizeof(planet_type));
		printf("Name of planet: ");
		scanf_s("%s", newPlanet->name, 20);
		sys = strcmp(newPlanet->name, "s");
		aut = strcmp(newPlanet->name, "a");
		if (sys == 0) { //Auto add system
			printf("Number of planets in system:");
			scanf_s("%d", &numb);
			autoAddSystem(numb);
		}
		else if (aut == 0) { // Auto add planet
			sprintf_s(newPlanet->name, 10, "%d", rand());
			newPlanet->mass = (rand() % 1000) * 100;
			newPlanet->sx = (rand() % 400) + 200;
			newPlanet->sy = (rand() % 300) + 150;
			if (rand() % 2 == 1)neg = -1;
			else neg = 1;
			newPlanet->vx = (rand() % 20) * 0.001 * neg;
			if (rand() % 2 == 1)neg = -1;
			else neg = 1;
			newPlanet->vy = (rand() % 20) * 0.001 * neg;
			newPlanet->life = rand() % 1000 + 500;
			sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());
		}

		else {	
			printf("Mass of planet (In thousands!): ");
			scanf_s("%lf", &(newPlanet->mass));
			newPlanet->mass = newPlanet->mass * 1000;
			printf("Position X: ");
			scanf_s("%lf", &(newPlanet->sx));
			printf("Position Y: ");
			scanf_s("%lf", &(newPlanet->sy));
			printf("Velocity X: ");
			scanf_s("%lf", &(newPlanet->vx));
			newPlanet->vx = newPlanet->vx * 0.001;
			printf("Velocity Y: ");
			scanf_s("%lf", &(newPlanet->vy));
			newPlanet->vy = newPlanet->vy * 0.001;
			printf("Life: ");
			scanf_s("%d", &(newPlanet->life));
			sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());
		}

		bytesWritten = mailslotWrite(serverMailSlot, newPlanet, sizeof(planet_type));
		if (bytesWritten != -1)printf("\nData sent to server (bytes = %d)\n", bytesWritten);
		else printf("\nFailed sending data to server\n");

		Sleep(500);

		free(newPlanet);
		newPlanet = NULL;

		

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


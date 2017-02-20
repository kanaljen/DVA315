#include <stdio.h>
#include <windows.h>
#include "wrapper.h"
#include "staffan.h"
#include <string.h>

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

void autoAddSystem(int numberOfPlanets)
{
	HANDLE serverMailSlot = connectToServerMailslot();
	time_t t;
	srand((unsigned)time(&t));
	int rc, neg;
	planet_type *newPlanet;

	newPlanet = (planet_type*)malloc(sizeof(planet_type));
	sprintf_s(newPlanet->name, 10, "%s", "Sun");
	newPlanet->mass = 100000000;
	newPlanet->sx = 400;
	newPlanet->sy = 300;
	newPlanet->vx = 0;
	newPlanet->vy = 0;
	newPlanet->life = 99999999;
	sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());

	mailslotWrite(serverMailSlot, newPlanet, sizeof(planet_type));
	Sleep(500);
	free(newPlanet);

	for (int i = 0; i < numberOfPlanets; i++) {

		newPlanet = (planet_type*)malloc(sizeof(planet_type));

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

		mailslotWrite(serverMailSlot, newPlanet, sizeof(planet_type));
		Sleep(500);
		free(newPlanet);
	}

	CloseHandle(serverMailSlot);

}

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../wrapper.h"
#include "database.h"


BOOL addPlanet(planet_type** database, planet_type** newPlanet)
{
	if (*database == NULL) //List is empty
	{
		*database = *newPlanet; //database points to the new planet
		(*database)->next = *database; //the new planet points to itself
		return TRUE;
	}
	else if ((*database)->next == *database) //There is one(1) planet in the list 
	{
		(*newPlanet)->next = *database; //The new planet points to the first planet, same as the db pointer
		(*database)->next = *newPlanet; //The first planet points to the new planet
		return TRUE;
	}
	else
	{
		(*newPlanet)->next = (*database)->next; //Take the first planets next
		(*database)->next = *newPlanet; //Point the first planet to the new one
		return TRUE;
	}
	return FALSE;
}

planet_type* searchList(const planet_type *currentNode, const char inquiredPlanet[20], const char firstPlanet[20]) {
	if (!lstrcmp(currentNode->name, inquiredPlanet))return currentNode; // If planet is found
	else if (!lstrcmp(currentNode->name, firstPlanet))return NULL; //If current node is the first planet, all planets have been checked
	return searchList(currentNode->next, inquiredPlanet, firstPlanet);
}

planet_type* findPlanet(const planet_type *database, const char planetName[20]) //Find out is list is empty and if the planet exist in db, and returns pointer to it
{
	if (database == NULL)return NULL;// If list is empty
	else if (!lstrcmp(database->name, planetName))return database; //If first planet is the inqured planet
	return searchList(database->next, planetName, database->name); //Go through list
}

BOOL removePlanet(planet_type** database, const char planetName[20])
{
	planet_type* planetToRemove = findPlanet(*database, planetName);
	planet_type* temp = NULL;
	if (planetToRemove == NULL)return FALSE; //List is empty, or planet does not exist
	if (*database == planetToRemove && (*database)->next == planetToRemove) { //Only one planet, remove it
		free(*database);
		*database = NULL;
		return TRUE;
	}

	while (TRUE) { // List har more than one planet and planettoremove exist in list

		if((*database)->next == planetToRemove){ //Planet to remove is the next planet
			temp = (*database)->next;
			(*database)->next = temp->next;
			free(temp);
			return TRUE;
		}
		*database = (*database)->next; //loop again
	}
	
}

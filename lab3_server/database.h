#ifndef DATABASE_H
#define DATABASE_H
#include "../wrapper.h"
#include <Windows.h>

extern BOOL addPlanet(planet_type** database, planet_type** newPlanet);
extern planet_type* searchList(const planet_type *currentNode, const char inquiredPlanet[20], const char firstPlanet[20]);
extern planet_type* findPlanet(const planet_type *database, const char planetName[20]);
extern BOOL removePlanet(planet_type *database, const char planetName[20]);

#endif

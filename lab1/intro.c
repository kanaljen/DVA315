#include <stdio.h>
#include "../wrapper.h"

void helloWorld(int reps) {
	if (reps == 0) {
		while (1)
		{
			printf("Hello World!\n");
			Sleep(200);
		}
	}
	else {
		for (int i = 0; i < reps; i++)
		{
			printf("Hello World!\n");
			Sleep(1000);
		}
	}

}

void helloMoon(int reps) { //Prints "Hello Moon!" unlimited number of times 
	if (reps == 0) {
		while (1)
		{
			printf("Hello Moon!\n");
			Sleep(200);
		}
	}
	else {
		for (int i = 0; i < reps; i++)
		{
			printf("Hello Moon!\n");
			Sleep(1000);
		}
	}

}

int main() {

	//Create handle-pointers
	PHANDLE handleTread_1 = NULL;
	PHANDLE handleTread_2 = NULL;

	//Start Hello Moon threads

	handleTread_1 = threadCreate((LPTHREAD_START_ROUTINE)helloMoon, 0);

	//Start Hello World threads

	handleTread_2 = threadCreate((LPTHREAD_START_ROUTINE)helloWorld, 0);

	getchar();
}
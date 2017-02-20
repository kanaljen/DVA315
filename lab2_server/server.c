/********************************************************************\
* server.c                                                           *
*                                                                    *
* Desc: example of the server-side of an application                 *
* Revised: Dag Nystrom & Jukka Maki-Turja                     *
*                                                                    *
* Based on generic.c from Microsoft.                                 *
*                                                                    *
*  Functions:                                                        *
*     WinMain      - Application entry point                         *
*     MainWndProc  - main window procedure                           *
*                                                                    *
* NOTE: this program uses some graphic primitives provided by Win32, *
* therefore there are probably a lot of things that are unfamiliar   *
* to you. There are comments in this file that indicates where it is *
* appropriate to place your code.                                    *
* *******************************************************************/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../wrapper.h"
#include "../staffan.h"
#include "database.h"

							/* the server uses a timer to periodically update the presentation window */
							/* here is the timer id and timer period defined                          */

#define UPDATE_FREQ     10	/* update frequency (in ms) for the timer */
#define G 0.0000000000667259


							/* (the server uses a mailslot for incoming client requests) */



/*********************  Prototypes  ***************************/
/* NOTE: Windows has defined its own set of types. When the   */
/*       types are of importance to you we will write comments*/ 
/*       to indicate that. (Ignore them for now.)             */
/**************************************************************/

LRESULT WINAPI MainWndProc( HWND, UINT, WPARAM, LPARAM );
DWORD WINAPI mailThread(LPARAM);
void WINAPI planetFunc(planet_type* planet);


planet_type* planetDatabase = NULL;


HDC hDC;		/* Handle to Device Context, gets set 1st time in MainWndProc */
				/* we need it to access the window for printing and drawin */

/********************************************************************\
*  Function: int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)    *
*                                                                    *
*   Purpose: Initializes Application                                 *
*                                                                    *
*  Comments: Register window class, create and display the main      *
*            window, and enter message loop.                         *
*                                                                    *
*                                                                    *
\********************************************************************/

							/* NOTE: This function is not too important to you, it only */
							/*       initializes a bunch of things.                     */
							/* NOTE: In windows WinMain is the start function, not main */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {

	HWND hWnd;
	DWORD threadID;
	MSG msg;


	/* Create the window, 3 last parameters important */
	/* The tile of the window, the callback function */
	/* and the backgrond color */

	hWnd = windowCreate(hPrevInstance, hInstance, nCmdShow, "Server", MainWndProc, COLOR_WINDOW + 1);

	/* start the timer for the periodic update of the window    */
	/* (this is a one-shot timer, which means that it has to be */
	/* re-set after each time-out) */
	/* NOTE: When this timer expires a message will be sent to  */
	/*       our callback function (MainWndProc).               */

	windowRefreshTimer(hWnd, UPDATE_FREQ);


	/* create a thread that can handle incoming client requests */
	/* (the thread starts executing in the function mailThread) */
	/* NOTE: See online help for details, you need to know how  */
	/*       this function does and what its parameters mean.   */
	/* We have no parameters to pass, hence NULL				*/


	threadID = threadCreate(mailThread, NULL);


	/* (the message processing loop that all windows applications must have) */
	/* NOTE: just leave it as it is. */
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}


/********************************************************************\
* Function: mailThread                                               *
* Purpose: Handle incoming requests from clients                     *
* NOTE: This function is important to you.                           *
/********************************************************************/
DWORD WINAPI mailThread(LPARAM arg) {

	DWORD msgSize;
	static int posY = 0;
	planet_type *buffer = NULL, *planetPtr = NULL;
	char *msg = (char*)malloc(sizeof(char)*100);

	//STARTUP
	HANDLE mailslot = mailslotCreate ("serverMailSlot");
	HANDLE startup = CreateEvent(NULL, 0, 0, "startup"), clientMailslot;
	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");
	SetEvent(startup);
	//END STARTUP

	while (TRUE) {

		do {
			GetMailslotInfo(mailslot, 0, &msgSize, 0, 0);
			Sleep(200);
		} while (msgSize == MAILSLOT_NO_MESSAGE);

		WaitForSingleObject(databaseMutex, INFINITE);
		planet_type* buffer = (planet_type*)malloc(sizeof(planet_type));
		mailslotRead(mailslot, buffer, sizeof(planet_type));
		if (buffer->life > 0) { //New planet from client
			planetPtr = findPlanet(planetDatabase, buffer->name);
			if (planetPtr != NULL) { //planet exist
				clientMailslot = mailslotConnect(buffer->pid);
				sprintf_s(&msg[0], 100, "[SERVER]: A planet with name '%s' already exist.", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
				clientMailslot = NULL;
				free(buffer);

			}
			else {//Add new planet

				clientMailslot = mailslotConnect(buffer->pid);
				if (!addPlanet(&planetDatabase, &buffer)) { //If somethings wrong with the add
					sprintf_s(&msg[0], 100, "[SERVER]: New planet '%s' NOT added to database.", buffer->name);
					mailslotWrite(clientMailslot, msg, strlen(msg));
				}
				else { //Report successfull add
					sprintf_s(&msg[0], 100, "[SERVER]: New planet '%s' added to database.", buffer->name);
					mailslotWrite(clientMailslot, msg, strlen(msg));
				}
				clientMailslot = NULL;
				threadCreate(planetFunc, buffer);

			}

		}
		else if (buffer->life < 0) { //Planet has left the system

			clientMailslot = mailslotConnect(buffer->pid);

			if (!removePlanet(&planetDatabase, buffer->name)) { //DB remove not ok
				sprintf_s(&msg[0], 100, "[SERVER]: Planet '%s' as left the area, but planet could NOT be removed!", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
			}

			else { //DB remove ok
				sprintf_s(&msg[0], 100, "[SERVER]: Planet '%s' as left the area, and planet has been removed!", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
			}
			clientMailslot = NULL;
			free(buffer);

		}
		else { // Life is zero

			clientMailslot = mailslotConnect(buffer->pid);

			if (!removePlanet(&planetDatabase, buffer->name)) { 
				sprintf_s(&msg[0], 100, "[SERVER]: Life of '%s' as expired, but planet could NOT be removed!", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
			}

			else {
				sprintf_s(&msg[0], 100, "[SERVER]: Life of '%s' as expired, and planet has been removed!", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
			}
			clientMailslot = NULL;
			free(buffer);

		}
		ReleaseMutex(databaseMutex);
		Sleep(200);
		buffer = NULL;
  }

  return 0;
}


void WINAPI planetFunc(planet_type* planet)
{

	HANDLE mailSlot, databaseMutex;
	static int posY = 0;
	char planetName[20];
	char testArray[50];
	double Fs = 0;
	double r = 0;
	double a1 = 0;
	double ax = 0;
	double ay = 0;
	double dt = 100;
	planet_type *nextPlanet;
	databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");
	WaitForSingleObject(databaseMutex, INFINITE);
	lstrcpy(planetName, planet->name);
	mailSlot = connectToServerMailslot();
	while (planet->life > 0)
	{
		nextPlanet = planet;
		
		while (lstrcmp(planetName, nextPlanet->next->name))
		{
			nextPlanet = nextPlanet->next;
			r = sqrt((pow(planet->sx - nextPlanet->sx, 2)) + pow(planet->sy - nextPlanet->sy, 2));
			a1 = G * ((nextPlanet->mass) / pow(r, 2));
			ax = ax + (a1 * (nextPlanet->sx - planet->sx) / r);
			ay = ay + (a1 * (nextPlanet->sy - planet->sy) / r);
		}
		planet->vx = planet->vx + ax * dt;
		planet->vy = planet->vy + ay * dt;
		planet->sx = planet->sx + planet->vx * dt;
		planet->sy = planet->sy + planet->vy * dt;
		planet->life--;
		
		if ((planet->sx < 0) || (planet->sx > 800) || (planet->sy < 0) || (planet->sy > 600))
			planet->life = -1;
		ReleaseMutex(databaseMutex);
		ax = 0;
		ay = 0;

		Sleep(100);
	}
	mailslotWrite(mailSlot, planet, sizeof(planet_type));
}


/********************************************************************\
* Function: LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM) *
*                                                                    *
* Purpose: Processes Application Messages (received by the window)   *
* Comments: The following messages are processed                     *
*                                                                    *
*           WM_PAINT                                                 *
*           WM_COMMAND                                               *
*           WM_DESTROY                                               *
*           WM_TIMER                                                 *
*                                                                    *
\********************************************************************/
/* NOTE: This function is called by Windows when something happens to our window */

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT ps;
	int posX;
	int posY;
	char life[10];
	HANDLE context;
	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");
	static DWORD color = 0;
	planet_type *firstPlanet = NULL, *currentPlanet = NULL;

	switch (msg) {
		/**************************************************************/
		/*    WM_CREATE:        (received on window creation)
		/**************************************************************/
	case WM_CREATE:
		hDC = GetDC(hWnd);
		break;
		/**************************************************************/
		/*    WM_TIMER:         (received when our timer expires)
		/**************************************************************/
	case WM_TIMER:

		/* NOTE: replace code below for periodic update of the window */
		/*       e.g. draw a planet system)                           */
		/* NOTE: this is referred to as the 'graphics' thread in the lab spec. */

		/* here we draw a simple sinus curve in the window    */
	
		WaitForSingleObject(databaseMutex, INFINITE);
		firstPlanet = planetDatabase;
		currentPlanet = firstPlanet;
		Rectangle(hDC, 0, 0, 800, 600);
		if (firstPlanet != NULL) {
			do {
				posX = currentPlanet->sx;
				posY = currentPlanet->sy;
				//SetPixel(hDC, posX, posY, (COLORREF)color);
				SetTextAlign(hDC, VTA_CENTER);
				sprintf_s(life, 10, "%d", currentPlanet->life);
				TextOut(hDC, posX ,posY + (log10(currentPlanet->mass) * 3.2),currentPlanet->name,lstrlen(currentPlanet->name));
				TextOut(hDC, posX, posY + (log10(currentPlanet->mass) * 3.2)+16, life, lstrlen(life));

				Ellipse(hDC, posX - (log10(currentPlanet->mass)*3), posY - (log10(currentPlanet->mass) * 3), posX+ (log10(currentPlanet->mass) * 3), posY+ (log10(currentPlanet->mass) * 3));
				color += 12;
				currentPlanet = currentPlanet->next;

			} while (currentPlanet != firstPlanet);
		}

		ReleaseMutex(databaseMutex);
		Sleep(0);
              
		windowRefreshTimer(hWnd, UPDATE_FREQ);
		break;
		/****************************************************************\
		*     WM_PAINT: (received when the window needs to be repainted, *
		*               e.g. when maximizing the window)                 *
		\****************************************************************/

	case WM_PAINT:
		/* NOTE: The code for this message can be removed. It's just */
		/*       for showing something in the window.                */
		context = BeginPaint(hWnd, &ps); /* (you can safely remove the following line of code) */

		EndPaint(hWnd, &ps);
		break;
		/**************************************************************\
		*     WM_DESTROY: PostQuitMessage() is called                  *
		*     (received when the user presses the "quit" button in the *
		*      window)                                                 *
		\**************************************************************/
	case WM_DESTROY:
		PostQuitMessage(0);
		/* NOTE: Windows will automatically release most resources this */
		/*       process is using, e.g. memory and mailslots.           */
		/*       (So even though we don't free the memory which has been*/
		/*       allocated by us, there will not be memory leaks.)      */

		ReleaseDC(hWnd, hDC); /* Some housekeeping */
		break;

		/**************************************************************\
		*     Let the default window proc handle all other messages    *
		\**************************************************************/
	default:
		return(DefWindowProc(hWnd, msg, wParam, lParam));
	}
	return 0;
}






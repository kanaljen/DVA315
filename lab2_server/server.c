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

							/* (the server uses a mailslot for incoming client requests) */



/*********************  Prototypes  ***************************/
/* NOTE: Windows has defined its own set of types. When the   */
/*       types are of importance to you we will write comments*/ 
/*       to indicate that. (Ignore them for now.)             */
/**************************************************************/

LRESULT WINAPI MainWndProc( HWND, UINT, WPARAM, LPARAM );
DWORD WINAPI mailThread(LPARAM);
void WINAPI planetFunc(planet_type* planetData);


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

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow ) {

	HWND hWnd;
	DWORD threadID;
	MSG msg;
	

							/* Create the window, 3 last parameters important */
							/* The tile of the window, the callback function */
							/* and the backgrond color */

	hWnd = windowCreate (hPrevInstance, hInstance, nCmdShow, "Server", MainWndProc,10);

							/* start the timer for the periodic update of the window    */
							/* (this is a one-shot timer, which means that it has to be */
							/* re-set after each time-out) */
							/* NOTE: When this timer expires a message will be sent to  */
							/*       our callback function (MainWndProc).               */
  
	windowRefreshTimer (hWnd, UPDATE_FREQ);
  

							/* create a thread that can handle incoming client requests */
							/* (the thread starts executing in the function mailThread) */
							/* NOTE: See online help for details, you need to know how  */ 
							/*       this function does and what its parameters mean.   */
							/* We have no parameters to pass, hence NULL				*/
  

	threadID = threadCreate (mailThread,NULL); 
  

							/* (the message processing loop that all windows applications must have) */
							/* NOTE: just leave it as it is. */
	while( GetMessage( &msg, NULL, 0, 0 ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
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
	SetEvent(startup);
	//END STARTUP

	while (TRUE) {

		do {
			GetMailslotInfo(mailslot, 0, &msgSize, 0, 0);
			//Sleep(200);
		} while (msgSize == MAILSLOT_NO_MESSAGE);

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
			else {
				addPlanet(&planetDatabase, &buffer);
				clientMailslot = mailslotConnect(buffer->pid);
				sprintf_s(&msg[0], 100, "[SERVER]: New planet '%s' added to database.", buffer->name);
				mailslotWrite(clientMailslot, msg, strlen(msg));
				clientMailslot = NULL;
				threadCreate(planetFunc, buffer);
			}

		}
		else {

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
		}

			buffer = NULL;
  }

  return 0;
}


void WINAPI planetFunc(planet_type* planetData)
{
	while (planetData->life > 0) {
		planetData->sx = planetData->sx + 10;
		planetData->sy = planetData->sy + 10;
		planetData->life = planetData->life - 1;
		Sleep(200);
	}
	HANDLE serverMailSlot = connectToServerMailslot();
	mailslotWrite(serverMailSlot, planetData, sizeof(planet_type));
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
	HANDLE context;
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
		/* just to show how pixels are drawn                  */
		
		firstPlanet = planetDatabase;
		currentPlanet = firstPlanet;

		if (firstPlanet != NULL) {
			do {
				posX = currentPlanet->sx;
				posY = currentPlanet->sy;
				SetPixel(hDC, posX, posY, 0);
				currentPlanet = currentPlanet->next;
			} while (currentPlanet != firstPlanet);
		}
		firstPlanet = NULL;
		currentPlanet = firstPlanet;

		windowRefreshTimer(hWnd, UPDATE_FREQ);
		break;
		/****************************************************************\
		*     WM_PAINT: (received when the window needs to be repainted, *
		*               e.g. when maximizing the window)                 *
		\****************************************************************/

	case WM_PAINT:
		/* NOTE: The code for this message can be removed. It's just */
		/*       for showing something in the window.                */

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






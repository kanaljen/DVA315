#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "wrapper.h"

#define TIMERID			100  /* id for timer that is used by the thread that manages the window where graphics is drawn */
#define DEFAULT_STACK_SIZE	1024
#define TIME_OUT			MAILSLOT_WAIT_FOREVER 

/* ATTENTION!!! calls that require a time out, use TIME_OUT constant, specifies that calls are blocked forever */


DWORD threadCreate (LPTHREAD_START_ROUTINE threadFunc, LPVOID threadParams) {

	/* Creates a thread running threadFunc */
	DWORD myThreadID;
	if (CreateThread(0, 0, threadFunc, threadParams, 0, &myThreadID) == INVALID_HANDLE_VALUE) printf("threadCreate error: %d\n", GetLastError);
	return myThreadID;
	/* optional parameters (NULL otherwise)and returns its id! */
}


HANDLE mailslotCreate (char *name) {

	/* Creates a mailslot with the specified name and returns the handle */
	char pathName[100];
	sprintf_s(&pathName[0], 100, "\\\\.\\mailslot\\%s", name);
	HANDLE mailH = CreateMailslot(pathName, 0, TIME_OUT, NULL);
	if (mailH == INVALID_HANDLE_VALUE)printf("mailCreate error: %d\n", GetLastError);
	else printf("Mailslot '%s' created...\n",name);
	return mailH;
	/* Should be able to handle a messages of any size */
}

HANDLE mailslotConnect (char * name) {

	/* Connects to an existing mailslot for writing */
	char pathName[100];
	sprintf_s(&pathName[0], 100, "\\\\.\\mailslot\\%s", name);
	HANDLE fileH = CreateFile(pathName,(GENERIC_READ | GENERIC_WRITE),FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (fileH == INVALID_HANDLE_VALUE)printf("mailslotConnect error: %d\n", GetLastError);
	else printf("Connected to '%s'...\n",name);
	return fileH;
	/* and returns the handle upon success     */
}

int mailslotWrite(HANDLE mailSlot, void *msg, int msgSize) {

	/* Write a msg to a mailslot, return nr */
	DWORD bytesWritten;
	BOOL fResult;
	fResult = WriteFile(mailSlot, msg, msgSize, &bytesWritten, NULL);
	if (!fResult)
	{
		printf("mailslotWrite error: %d.\n", GetLastError());
		return 0;
	}

	return bytesWritten;

	/* of successful bytes written         */
}

int	mailslotRead (HANDLE mailbox, void *msg, int msgSize) {

	/* Read a msg from a mailslot, return nr */
	int bytesRead = 0;
	ReadFile(mailbox, msg, msgSize, &bytesRead, NULL);
	return bytesRead;
	/* of successful bytes read              */
}

int mailslotClose(HANDLE mailSlot){
	
	/* close a mailslot, returning whatever the service call returns */
	if (CloseHandle(mailSlot) == 0)printf("mailslotClose error: %d\n",GetLastError());
	return 0;
	
}


/******************** Wrappers for window management, used for lab 2 and 3 ***********************/
/******************** DONT CHANGE!!! JUST FYI ******************************************************/


HWND windowCreate (HINSTANCE hPI, HINSTANCE hI, int ncs, char *title, WNDPROC callbackFunc, int bgcolor) {

  HWND hWnd;
  WNDCLASS wc; 

  /* initialize and create the presentation window        */
  /* NOTE: The only important thing to you is that we     */
  /*       associate the function 'MainWndProc' with this */
  /*       window class. This function will be called by  */
  /*       windows when something happens to the window.  */
  if( !hPI) {
	 wc.lpszClassName = "GenericAppClass";
	 wc.lpfnWndProc = callbackFunc;          /* (this function is called when the window receives an event) */
	 wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	 wc.hInstance = hI;
	 wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	 wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	 wc.hbrBackground = (HBRUSH) bgcolor;
	 wc.lpszMenuName = "GenericAppMenu";

	 wc.cbClsExtra = 0;
	 wc.cbWndExtra = 0;

	 RegisterClass( &wc );
  }

  /* NOTE: This creates a window instance. Don't bother about the    */
  /*       parameters to this function. It is sufficient to know     */
  /*       that this function creates a window in which we can draw. */
  hWnd = CreateWindow( "GenericAppClass",
				 title,
				 WS_OVERLAPPEDWINDOW,//|WS_HSCROLL|WS_VSCROLL,
				 0,
				 0,
				 816,//CW_USEDEFAULT,
				 639,//CW_USEDEFAULT,
				 NULL,
				 NULL,
				 hI,
				 NULL
				 );

  /* NOTE: This makes our window visible. */
  ShowWindow( hWnd, ncs );
  /* (window creation complete) */

  return hWnd;
}

void windowRefreshTimer (HWND hWnd, int updateFreq) {

  if(SetTimer(hWnd, TIMERID, updateFreq, NULL) == 0) {
	 /* NOTE: Example of how to use MessageBoxes, see the online help for details. */
	 MessageBox(NULL, "Failed setting timer", "Error!!", MB_OK);
	 exit (1);
  }
}


/******************** Wrappers for window management, used for lab  3 ***********************/
/*****  Lab 3: Check in MSDN GetOpenFileName and GetSaveFileName  *********/
/**************  what the parameters mean, and what you must call this function with *********/


HANDLE OpenFileDialog(char* string, DWORD accessMode, DWORD howToCreate)
{

	OPENFILENAME opf;
	char szFileName[_MAX_PATH]="";

	opf.Flags				= OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 
	opf.lpstrDefExt			= "dat";
	opf.lpstrCustomFilter	= NULL;
	opf.lStructSize			= sizeof(OPENFILENAME);
	opf.hwndOwner			= NULL;
	opf.lpstrFilter			= NULL;
	opf.lpstrFile			= szFileName;
	opf.nMaxFile			= _MAX_PATH;
	opf.nMaxFileTitle		= _MAX_FNAME;
	opf.lpstrInitialDir		= NULL;
	opf.lpstrTitle			= string;
	opf.lpstrFileTitle		= NULL ; 
	
	if(accessMode == GENERIC_READ)
		GetOpenFileName(&opf);
	else
		GetSaveFileName(&opf);

	return CreateFile(szFileName, 
		accessMode, 
		0, 
		NULL, 
		howToCreate, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);


}


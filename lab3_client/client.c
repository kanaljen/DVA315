#include <windows.h>
#include <Windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "resource.h"
#include "../wrapper.h"
#include "database.h"

planet_type* planetDatabase = NULL;
HWND databaseDLG, feedbackDLG, addDLG, systemDLG;
HANDLE serverMailSlot;


LRESULT CALLBACK WndProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK planetListPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK addPlanetPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK addSystemPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI mailThread(LPARAM);

void newAutoAddSystem(int numOfPlanets);
void openPlanetFile(void);
void savePlanetFile(void);



INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	MSG        Msg;
	WNDCLASSEX WndClsEx;
	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	

	// Create the application window
	WndClsEx.cbSize = sizeof(WNDCLASSEX);
	WndClsEx.style = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc = WndProcedure;
	WndClsEx.cbClsExtra = 0;
	WndClsEx.cbWndExtra = 0;
	WndClsEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClsEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClsEx.lpszMenuName = (LPCSTR)MAKEINTRESOURCEW(IDR_MENU1);
	WndClsEx.lpszClassName = "Planet Client Application";
	WndClsEx.hInstance = hInstance;
	WndClsEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Register the application
	RegisterClassEx(&WndClsEx);

	// Create the mainwindow object
	CreateWindowEx(0, "Planet Client Application", "Client",
		(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |WS_CLIPCHILDREN | WS_VISIBLE), 
		CW_USEDEFAULT, CW_USEDEFAULT, 816, 639, NULL, NULL, hInstance, NULL);
	
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);

	serverMailSlot = mailslotConnect("serverMailSlot");
	if (serverMailSlot == INVALID_HANDLE_VALUE) {
		SendMessage(feedbackHandle, LB_ADDSTRING, NULL, "[CLIENT] Failed to connect to server");
	}
	else SendMessage(feedbackHandle, LB_ADDSTRING, NULL, "[CLIENT] Connected to server");



	//Windows msg loop
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

DWORD WINAPI mailThread()
{
	
	char processID[10];

	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	char strbuffer[100];

	SendMessage(feedbackHandle, LB_ADDSTRING, NULL, "[CLIENT] Mailthread started");

	sprintf_s(processID, 10, "%d", GetCurrentProcessId()); //Convert ProcessID int to char
	HANDLE mailSlot = mailslotCreate(processID);
	if (mailSlot == INVALID_HANDLE_VALUE) {
		SendMessage(feedbackHandle, LB_ADDSTRING, NULL, "[CLIENT] Failed to get create client mailslot");
		return;
	}
	//END Start client-mailslot

	int msgSize;
	feedback_type *msg;
	while (TRUE) {
		GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0); //RETRIVE SIZE OF MSG
		if (msgSize > 0) {

			//Retrive msg
			msg = (feedback_type*)malloc(sizeof(feedback_type));
			mailslotRead(mailSlot, msg, sizeof(feedback_type));

			// Print feedback messege
			SendMessage(feedbackHandle, LB_ADDSTRING, NULL, msg->msg);

			// Move planets, from active to waiting
			if (msg->type == 0) {

				SendMessage(alivelistHandle, LB_DELETESTRING, 0, msg->name);
				SendMessage(databaselistHandle, LB_ADDSTRING, NULL, msg->name);
				sprintf_s(strbuffer, 100, "[CLIENT] Added '%s' to local database", msg->name);
				SendMessage(feedbackHandle, LB_ADDSTRING, NULL, strbuffer);

			}

			// Free memory and stuff
			free(msg);
			msg = NULL;
		}
		Sleep(200);
	};
	return 0;
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT Msg,
	WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInstance = GetModuleHandle(0);

	
	switch (Msg)
	{
	case WM_CREATE:
		// Create input window object 
		databaseDLG = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DATABASEDLG), hWnd, planetListPROC);
		feedbackDLG = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FEEDBACKDLG), hWnd, planetListPROC);

		ShowWindow(feedbackDLG, 1);
		ShowWindow(databaseDLG, 1);

		DWORD threadID = threadCreate(mailThread, NULL);

		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_MENU1_NEWPLANET:
			addDLG = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ADDPLANET), hWnd, addPlanetPROC);
			ShowWindow(addDLG, 1);
			break;
		case ID_MENU1_NEWSYSTEM:
			systemDLG = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ADDSYSTEM), hWnd, addSystemPROC);
			ShowWindow(systemDLG, 1);
			break;
		case ID_PLANET_OPENPLANET:
			openPlanetFile();
			break;
		case ID_PLANET_SAVEPLANET:
			savePlanetFile();
			break;
		case ID_PLANET_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		// then close it
		PostQuitMessage(WM_QUIT);
		break;
	default:
		// Process the left-over messages
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	// If something was not done, let it go
	return 0;
}

INT_PTR CALLBACK addPlanetPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);
	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)// || LOWORD(wParam) == IDCANCEL)
		{
			planet_type *buffer = (planet_type*)malloc(sizeof(planet_type));
			char *strbuffer = (char*)malloc(sizeof(char) * 10);
			
			//Fill buffer with new planet data
			sprintf_s(buffer->pid, 10, "%d", GetCurrentProcessId());
			buffer->next = NULL;
			GetDlgItemText(hDlg, IDC_EDIT_NAME,buffer->name,20);
			if (findPlanet(planetDatabase, buffer->name) != NULL) {
				free(buffer);
				return (INT_PTR)FALSE;
			}
			GetDlgItemText(hDlg, IDC_EDIT_MASS, strbuffer, 10);
			buffer->mass = atoi(strbuffer);
			GetDlgItemText(hDlg, IDC_EDIT_LIFE, strbuffer, 10);
			buffer->life = atoi(strbuffer);
			GetDlgItemText(hDlg, IDC_EDIT_XPOS, strbuffer, 3);
			buffer->sx = atoi(strbuffer);
			GetDlgItemText(hDlg, IDC_EDIT_YPOS, strbuffer, 3);
			buffer->sy = atoi(strbuffer);
			GetDlgItemText(hDlg, IDC_EDIT_XVELOC, strbuffer, 3);
			buffer->vx = atoi(strbuffer);
			GetDlgItemText(hDlg, IDC_EDIT_YVELOC, strbuffer, 3);
			buffer->vy = atoi(strbuffer);
			strbuffer = NULL;

			WaitForSingleObject(databaseMutex,INFINITE);
			//Add new planet to database
			addPlanet(&planetDatabase,&buffer);
			ReleaseMutex(databaseMutex);
			char *newstrbuffer[100];
			sprintf_s(newstrbuffer, 100, "[CLIENT] Added '%s' to local database", buffer->name);
			SendMessage(feedbackHandle, LB_ADDSTRING, NULL, newstrbuffer);
			SendMessage(databaselistHandle, LB_ADDSTRING, NULL, buffer->name);

			// Free memory
			buffer = NULL;
			free(strbuffer);

			//End Dialog
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK addSystemPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)// || LOWORD(wParam) == IDCANCEL)
		{

			char *strbuffer = (char*)malloc(sizeof(char) * 10);

			//How many planets?
			GetDlgItemText(hDlg, IDC_NUMBEROFPLAN, strbuffer, 10);
			int numOfPlanets = atoi(strbuffer);

			// Free memory
			strbuffer = NULL;
			free(strbuffer);

			threadCreate(newAutoAddSystem,numOfPlanets);

			//End Dialog
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK planetListPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON_SENDPLANET)
		{
			planet_type *planetToSend;
			int *selItemsArray = (int*)malloc(sizeof(int) * 10);
			//Count number of selected items in list
			int numSelItems = SendMessage(databaselistHandle, LB_GETSELCOUNT, 0, 0);
			//Write selected items index# to an array
			int cSelItemsInBuffer = SendMessage(databaselistHandle, LB_GETSELITEMS, 10, (LPARAM)selItemsArray);
			
			char *nameOfPlanet;
			int k = 0;
			WaitForSingleObject(databaseMutex, INFINITE);
			for (int i = 0; i < numSelItems; i++) {
				
				nameOfPlanet = (char*)malloc(sizeof(char) * 20);
				SendMessage(databaselistHandle, LB_GETTEXT, selItemsArray[i]-k, nameOfPlanet);
				planetToSend = findPlanet(planetDatabase,nameOfPlanet);

				//Remove planet name from localdatabase list
				SendMessage(databaselistHandle, LB_DELETESTRING, selItemsArray[i]-k, 0);

				//Add planet name to active planet list
				SendMessage(alivelistHandle, LB_ADDSTRING, NULL, nameOfPlanet); 

				char txtbuffer[100];
				int bytesWritten = mailslotWrite(serverMailSlot, planetToSend, sizeof(planet_type));
				if (bytesWritten != -1) {
					sprintf_s(txtbuffer, 100, "[CLIENT] Planet '%s' sent to server", planetToSend->name);
					SendMessage(feedbackHandle, LB_ADDSTRING, NULL, txtbuffer);
				}
				else SendMessage(feedbackHandle, LB_ADDSTRING, NULL, "[CLIENT] Failed to write to server"); 
				free(nameOfPlanet);
				k++;
				Sleep(50);
			}
			ReleaseMutex(databaseMutex);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON_REMPLANET)
		{

			int *selItemsArray = (int*)malloc(sizeof(int) * 100);
			//Count number of selected items in list
			int numSelItems = SendMessage(databaselistHandle, LB_GETSELCOUNT, 0, 0);
			//Write selected items index# to an array
			int cSelItemsInBuffer = SendMessage(databaselistHandle, LB_GETSELITEMS, 100, (LPARAM)selItemsArray);

			char *nameOfPlanet;
			int k = 0;
			WaitForSingleObject(databaseMutex, INFINITE);
			for (int i = 0; i < numSelItems; i++) {

				nameOfPlanet = (char*)malloc(sizeof(char) * 20);
				SendMessage(databaselistHandle, LB_GETTEXT, selItemsArray[i] - k, nameOfPlanet);
				
				//Remove planet name from localdatabase list
				SendMessage(databaselistHandle, LB_DELETESTRING, selItemsArray[i] - k, 0);

				
				//Remove planet from database
				removePlanet(&planetDatabase,nameOfPlanet);
				

				char txtbuffer[100];
				sprintf_s(txtbuffer, 100, "[CLIENT] Removed '%s' from local database", nameOfPlanet);
				SendMessage(feedbackHandle, LB_ADDSTRING, NULL, txtbuffer);

				free(nameOfPlanet);
				k++;
				Sleep(50);
			}
			ReleaseMutex(databaseMutex);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void newAutoAddSystem(int numOfPlanets) {
	
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	time_t t;
	srand((unsigned)time(&t));
	int rc, neg;
	planet_type *newPlanet;


	//Create the SUN
	newPlanet = (planet_type*)malloc(sizeof(planet_type));
	sprintf_s(newPlanet->name, 10, "%s", "Sun");
	newPlanet->mass = 100000000;
	newPlanet->sx = 400;
	newPlanet->sy = 300;
	newPlanet->vx = 0;
	newPlanet->vy = 0;
	newPlanet->life = 99999999;
	sprintf_s(newPlanet->pid, 10, "%d", GetCurrentProcessId());

	WaitForSingleObject(databaseMutex, INFINITE);
	//Add sun to database
	addPlanet(&planetDatabase, &newPlanet);
	char *newstrbuffer[100];
	sprintf_s(newstrbuffer, 100, "[CLIENT] Added '%s' to local database", newPlanet->name);
	SendMessage(feedbackHandle, LB_ADDSTRING, NULL, newstrbuffer);
	SendMessage(databaselistHandle, LB_ADDSTRING, NULL, newPlanet->name);
	ReleaseMutex(databaseMutex);

	newPlanet = NULL;
	Sleep(50);

	for (int i = 0; i < numOfPlanets; i++) {

		newPlanet = (planet_type*)malloc(sizeof(planet_type));

		sprintf_s(newPlanet->name, 10, "%d", rand());
		if (findPlanet(planetDatabase, newPlanet->name) != NULL) {
			free(newPlanet);
			newPlanet = NULL;
			i--;
		}
		else {
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

			WaitForSingleObject(databaseMutex, INFINITE);
			//Add new planet to database
			addPlanet(&planetDatabase, &newPlanet);
			char *newstrbuffer[100];
			sprintf_s(newstrbuffer, 100, "[CLIENT] Added '%s' to local database", newPlanet->name);
			SendMessage(feedbackHandle, LB_ADDSTRING, NULL, newstrbuffer);
			SendMessage(databaselistHandle, LB_ADDSTRING, NULL, newPlanet->name);
			ReleaseMutex(databaseMutex);
			newPlanet = NULL;
			Sleep(50);
		}

	}
}

void openPlanetFile(void){
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	HANDLE fileHandle = OpenFileDialog("Open Planet", GENERIC_READ, OPEN_EXISTING);


	if (fileHandle == INVALID_HANDLE_VALUE)return;


	planet_type *newPlanet;

	

	int totalBytes = 0;
	PLARGE_INTEGER fileSize = calloc(1,sizeof(LARGE_INTEGER));
	GetFileSizeEx(fileHandle,fileSize);
	int filesToRead = (fileSize->LowPart / (sizeof(planet_type)));
	free(fileSize);

	WaitForSingleObject(databaseMutex, INFINITE);
	
	for (int i = 0; i < filesToRead; i++) {

		newPlanet = calloc(1,sizeof(planet_type));

		int bytesWritten;

		ReadFile(fileHandle, (LPVOID)newPlanet, sizeof(planet_type), &bytesWritten, NULL);

		totalBytes = totalBytes + bytesWritten;
		
		addPlanet(&planetDatabase,&newPlanet);

		char *newstrbuffer[100];
		sprintf_s(newstrbuffer, 100, "[CLIENT] Added '%s' to local database", newPlanet->name);
		SendMessage(feedbackHandle, LB_ADDSTRING, NULL, newstrbuffer);
		SendMessage(databaselistHandle, LB_ADDSTRING, NULL, newPlanet->name);

		newPlanet = NULL;

		Sleep(50);
	}
	ReleaseMutex(databaseMutex);

	char txtbuffer[100];
	sprintf_s(txtbuffer, 100, "[CLIENT] %d bytes read from file", totalBytes);
	SendMessage(feedbackHandle, LB_ADDSTRING, NULL, txtbuffer);

	CloseHandle(fileHandle);

}

void savePlanetFile(void) {
	HWND feedbackHandle = GetDlgItem(feedbackDLG, IDC_FEEDBACKLIST);
	HWND databaselistHandle = GetDlgItem(databaseDLG, IDC_PLANETLIST);
	HWND alivelistHandle = GetDlgItem(databaseDLG, IDC_ALIVELIST);

	HANDLE databaseMutex = CreateMutex(NULL, FALSE, "accessToDatabase");

	HANDLE fileHandle = OpenFileDialog("Save Planet", GENERIC_WRITE, CREATE_ALWAYS);

	if (fileHandle == INVALID_HANDLE_VALUE)return;

	planet_type *planetToSend;

	int *selItemsArray = (int*)malloc(sizeof(int) * 10);
	//Count number of selected items in list
	int numSelItems = SendMessage(databaselistHandle, LB_GETSELCOUNT, 0, 0);
	//Write selected items index# to an array
	int cSelItemsInBuffer = SendMessage(databaselistHandle, LB_GETSELITEMS, 10, (LPARAM)selItemsArray);

	int totalBytes = 0;
	char *nameOfPlanet;

	WaitForSingleObject(databaseMutex, INFINITE);
	for (int i = 0; i < numSelItems; i++) {

		nameOfPlanet = (char*)malloc(sizeof(char) * 20);
		SendMessage(databaselistHandle, LB_GETTEXT, selItemsArray[i], nameOfPlanet);
		planetToSend = findPlanet(planetDatabase, nameOfPlanet);

		int bytesWritten;
		WriteFile(fileHandle, planetToSend, sizeof(planet_type), &bytesWritten, NULL);

		totalBytes = totalBytes + bytesWritten;

		free(nameOfPlanet);

		Sleep(50);
	}
	ReleaseMutex(databaseMutex);

	char txtbuffer[100];
	sprintf_s(txtbuffer, 100, "[CLIENT] %d bytes written to file", totalBytes);
	SendMessage(feedbackHandle, LB_ADDSTRING, NULL, txtbuffer);

	CloseHandle(fileHandle);
}
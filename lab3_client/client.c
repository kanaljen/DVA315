#include <windows.h>
#include <Windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "../wrapper.h"
#include "database.h"

planet_type* planetDatabase;
HWND databaseDLG, feedbackDLG;
HANDLE serverMailSlot;


LRESULT CALLBACK WndProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK planetListPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK addPlanetPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI mailThread(LPARAM);



INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	MSG        Msg;
	WNDCLASSEX WndClsEx;

	

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
			if (msg->type == 0 || msg->type == -1) {

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

		ShowWindow(databaseDLG,1);
		ShowWindow(feedbackDLG, 1);

		DWORD threadID = threadCreate(mailThread, NULL);

		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_MENU1_NEWPLANET:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ADDPLANET), hWnd, addPlanetPROC);
			break;
		case ID_MENU1_NEWSYSTEM:
			//System add
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

			//Add new planet to database
			addPlanet(&planetDatabase,&buffer);
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

INT_PTR CALLBACK planetListPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
			}

			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON_REMPLANET)
		{

			int *selItemsArray = (int*)malloc(sizeof(int) * 10);
			//Count number of selected items in list
			int numSelItems = SendMessage(databaselistHandle, LB_GETSELCOUNT, 0, 0);
			//Write selected items index# to an array
			int cSelItemsInBuffer = SendMessage(databaselistHandle, LB_GETSELITEMS, 10, (LPARAM)selItemsArray);

			char *nameOfPlanet;
			int k = 0;
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
			}

			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


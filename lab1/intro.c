#include <stdio.h>
#include <string.h>
#include "../wrapper.h"

HANDLE mailSlot;
INT shutdownFlag = 0;

struct mailStruct{
	char*  head;
	char*  body;
};

DWORD WINAPI helloWorld(LPVOID rep);
DWORD WINAPI helloMoon(LPVOID rep);
DWORD WINAPI mailServer(char* name);
DWORD WINAPI inputClient(char* name);
DWORD WINAPI outputClient(char* name);

int main() {
	
	//Mailbox name input
	char name[10];
	int c;
	printf("Mailbox name (max 9 char): ");
	while (scanf_s("%9s", name, 10) != 1);
	while ((c = getchar()) != '\n' && c != EOF) {}
	

	//Create event-objects for multithread sync
	if (CreateEvent(NULL, 0, 0, "YouGotMail") == INVALID_HANDLE_VALUE)printf("YouGotMail-event error: %d", GetLastError);
	if (CreateEvent(NULL, 0, 0, "msgRead") == INVALID_HANDLE_VALUE)printf("msgRead-event error: %d", GetLastError);
	if (CreateEvent(NULL, 0, 0, "serverStartup") == INVALID_HANDLE_VALUE)printf("serverStartup-event error: %d", GetLastError);

	//Start threads
	DWORD threadID[3];
	threadID[0] = threadCreate(&mailServer,name); //Start server thread
	HANDLE startupEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "serverStartup");
	WaitForSingleObject(startupEventH, INFINITE);
	threadID[2] = threadCreate(&outputClient, name); //Start input-client thread
	WaitForSingleObject(startupEventH, INFINITE);
	threadID[1] = threadCreate(&inputClient, name); //Start input-client thread
	CloseHandle(startupEventH);

	//Get handles for threads
	HANDLE threadHandles[3];
	for (size_t i = 0; i < 3; i++)
	{
		threadHandles[i] = OpenThread(THREAD_ALL_ACCESS, 0, threadID[i]);
	}
	
	//Wait for thread handles
	WaitForMultipleObjects(3,threadHandles,TRUE,INFINITE);

	//Exit program
	Sleep(500);
	mailslotClose(mailSlot);
	printf("\nMailslot Terminated...\n");
	Sleep(500);
	for (size_t i = 0; i < 3; i++)
	{
		CloseHandle(threadHandles[i]);
	}
	printf("Thread-handles closed...\n\n");
	printf("Press any key to exit...");
	getchar();
	
	/*  FÖRSTA DELEN AV LABBEN
	HANDLE activeThread;
	DWORD threadID[2];

	threadID[0] = threadCreate(&helloMoon, 0); 	//START THREAD 1
	activeThread = OpenThread(THREAD_ALL_ACCESS, 0, threadID[0]); //GET HANDLE FOR THREAD 1
	WaitForSingleObject(activeThread,INFINITE); // WAIT FOR THREAD 1 TO TERMINATE
	CloseHandle(activeThread);

	threadID[1] = threadCreate(&helloWorld, 0);	//START THREAD 2
	activeThread = OpenThread(THREAD_ALL_ACCESS, 0, threadID[1]); //GET HANDLE FOR THREAD 2
	WaitForSingleObject(activeThread, INFINITE); // WAIT FOR THREAD 2 TO TERMINATE
	CloseHandle(activeThread);
	*/
	
}

DWORD WINAPI helloWorld(LPVOID rep) {
	int reps = 10;

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

	return 0;

}

DWORD WINAPI helloMoon(LPVOID rep) {
	int reps = 10;
	
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

	return 0;

}

DWORD WINAPI mailServer(char* name){
	

	mailSlot = mailslotCreate(name);

	HANDLE recivedEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "YouGotMail");
	HANDLE startupEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "serverStartup");
	
	int msgSize = 0;
	BOOL retrieveMsg = TRUE;
	printf("Server started...\n");

	SetEvent(startupEventH);
	CloseHandle(startupEventH);

	while (shutdownFlag != 1)
	{


		retrieveMsg = GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0);

		if (!retrieveMsg)
			printf("GetMailslotInfo error: %d\n", GetLastError());
		else if (msgSize == MAILSLOT_NO_MESSAGE)
		{

		}
		else
		{
		
			SetEvent(recivedEventH);
			
		}
		
		Sleep(2000);

	}
	if(shutdownFlag == 1)SetEvent(recivedEventH);
	CloseHandle(recivedEventH);
	return 0;
}

DWORD WINAPI inputClient(char* name) {
	
	HANDLE fileHandle = mailslotConnect(name);

	HANDLE msgreadEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "msgRead");

	struct mailStruct *msg = (struct mailStruct*)malloc(sizeof(struct mailStruct)); //POINTER TO MSG-STRUCT
	msg->head = malloc(sizeof(char) * 20); //Memory for head of msg
	msg->body = malloc(sizeof(char) * 50); //Memory for body of msg
	int bwritten;
	
	printf("Input-client started...\n");
	
	
	//SEND MSG PROMPT, INPUT-LOOP
	while(1){

		printf("\n(IC) Message head: ");
		gets_s(msg->head,20);
		if (strcmp(msg->head, "END") == 0)break;
		printf("(IC) Message body: ");
		gets_s(msg->body, 50);
		bwritten = mailslotWrite(fileHandle, msg->head, strlen(msg->head));
		bwritten = bwritten + mailslotWrite(fileHandle, msg->body, strlen(msg->body));
		printf("(IC) %d bytes written to slot.\n", bwritten);
		WaitForSingleObject(msgreadEventH,INFINITE);
	}
	free(msg->head);
	free(msg->body);
	free(msg);
	shutdownFlag = 1; //SIGNAL THREAD SHUTDOWN
	CloseHandle(msgreadEventH);
	return 0;
}

DWORD WINAPI outputClient(char* name) {

	struct mailStruct *msg = (struct mailStruct*)malloc(sizeof(struct mailStruct)); //POINTER TO MSG-STRUCT
	msg->head = (char*)malloc(sizeof(char) * 20); //Memory for head of msg
	msg->body = (char*)malloc(sizeof(char) * 50); //Memory for body of msg

	HANDLE recivedEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "YouGotMail"); //HANDLE FOR EVENT TO ACTIVATE OCLIENT
	HANDLE msgreadEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "msgRead"); 
	HANDLE startupEventH = OpenEvent(EVENT_ALL_ACCESS, 0, "serverStartup");
	
	int msgSize = 0;
	int bread = 0;
	printf("Output-client started...\n");
	SetEvent(startupEventH);
	CloseHandle(startupEventH);

	while (1) {

		WaitForSingleObject(recivedEventH, INFINITE); //WAITING FOR SERVER TO SET EVENT
		
		if (shutdownFlag == 1)break; //GLOBAL SHUTDOWN SIGNAL

		GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0); //RETRIVE SIZE OF MSG
		bread = mailslotRead(mailSlot, msg->head, msgSize);
		msg->head[msgSize] = '\0';
		GetMailslotInfo(mailSlot, 0, &msgSize, 0, 0);
		bread = bread + mailslotRead(mailSlot, msg->body, msgSize);
		msg->body[msgSize] = '\0';
		printf("\n(UC) Message head: %s", msg->head);
		printf("\n(UC) Message body: %s\n", msg->body);
		printf("(UC) %d read from slot.\n", bread);
		SetEvent(msgreadEventH);

	}
	free(msg->head);
	free(msg->body);
	free(msg);
	CloseHandle(recivedEventH);
	CloseHandle(msgreadEventH);
	return 0;
};
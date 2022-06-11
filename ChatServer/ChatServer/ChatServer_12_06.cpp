// ChatServer_12_06.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

char* ids[64];
SOCKET clients[64];
int numClients;


DWORD WINAPI ClientThread(LPVOID);


int main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);

	numClients = 0;

	while (1)
	{
		printf("Waiting for new client...\n");
		SOCKET client = accept(listener, NULL, NULL);
		printf("New client accepted: %d\n", client);

		CreateThread(0, 0, ClientThread, &client, 0, 0);
		const char* okMsg = "Xin chao. Hay nhap id theo cu phap \"[CONNECT] [your_id]\".\n";
		send(client, okMsg, strlen(okMsg), 0);
	}

}

DWORD WINAPI ClientThread(LPVOID lpParam)
{
	SOCKET client = *(SOCKET*)lpParam;

	char buf[256];
	int ret;

	char cmd[16], id[32], tmp[32], add[16];

	const char* errorMsg = "no such command \n";

	BOOL isRegistered = FALSE;
	bool isIdExisted = false;

	while (1)
	{
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0)
		{
			closesocket(client);
			return 0;
		}

		buf[ret] = 0;
		printf("Received: %s\n", buf);

		if (isRegistered == FALSE)
		{
			char sendBuf[256];
			char sendBufSelf[256];

			// Xu ly du lieu
			ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
			if (ret == 2)
			{
				if (strcmp(cmd, "[CONNECT]") == 0)
				{

					//check if id exists 
					for (int i = 0; i < numClients; i++)
					{
						if (strcmp(id, ids[i]) == 0)
						{
							isIdExisted = true;
							errorMsg = " id existed \n";
							strcpy(sendBuf, "[CONNECT] ERROR ");
							strcat(sendBuf, errorMsg);
							send(client, sendBuf, strlen(sendBuf), 0);
							break;
						}
					}

					// if id doesnt exist
					if (isIdExisted == false)
					{
						strcpy(sendBufSelf, "[CONNECT] OK \n ");
						send(client, sendBufSelf, strlen(sendBufSelf), 0);

						strcpy(sendBuf, "CONNECT ");
						strcat(sendBuf, buf + strlen(cmd) + 1);
						for (int i = 0; i < numClients; i++) {
							if (clients[i] != client)
							{
								send(clients[i], sendBuf, strlen(sendBuf), 0);
							}

						}
						// Luu thong tin cua client dang nhap thanh cong
						ids[numClients] = id;
						clients[numClients] = client;
						numClients++;
						isRegistered = TRUE;

					}

				}
				else
				{
					strcpy(sendBuf, "[CONNECT] ERROR \n ");
					strcat(sendBuf, errorMsg);
					send(client, sendBuf, strlen(sendBuf), 0);
				}
			}
			else
			{
				send(client, errorMsg, strlen(errorMsg), 0);
			}
		}
		//if registered 
		else
		{
			// Forward messages

			ret = sscanf(buf, "%s %s", cmd, add);
			if (ret < 1)
			{
				send(client, errorMsg, strlen(errorMsg), 0);
				continue;
			}

			char sendBuf[256];
			char sendBufSelf[256];

			if (ret == 1)
			{
				//if user find list
				if (strcmp(cmd, "[LIST]") == 0)
				{
					strcpy(sendBuf, "[LIST] OK ");
					for (int i = 0; i < numClients; i++) {
						strcat(sendBuf, ids[i]);
						strcat(sendBuf, ", ");
					}
					strcat(sendBuf, "\n"); // xuong dong
					send(client, sendBuf, strlen(sendBuf), 0);
				}

				//if user log out
				else if (strcmp(cmd, "[DISCONNECT]") == 0)
				{
					isRegistered = FALSE;
					for (int i = 0; i < numClients; i++) {

						//thong bao user khac
						if (clients[i] != client)
						{
							strcpy(sendBuf, "DISCONNECT ");
							strcat(sendBuf, buf + strlen(cmd) + 1);   // tên mảng trong c là 1 con trỏ, có địa chỉ bằng phần tử đầu tiên của mảng
							send(clients[i], sendBuf, strlen(sendBuf), 0);
						}

						else {
							strcpy(sendBufSelf, "[DISCONNECT] OK \n");
							send(client, sendBufSelf, strlen(sendBufSelf), 0);
						}


					}
				}
				// typing the wrong command, not list or disconnect
				else
				{
					send(client, errorMsg, strlen(errorMsg), 0);
				}
			}


			if (strcmp(cmd, "[SEND]") == 0)
			{
				// if the command is SEND ALL
				if (strcmp(add, "ALL") == 0)
				{
					strcpy(sendBuf, "[MESSAGE] ALL ");
					strcat(sendBuf, buf + strlen(cmd) + 1 + strlen(id) + 1); // get user message from buf: send all message

					for (int i = 0; i < numClients; i++)
						if (clients[i] != client)
							send(clients[i], sendBuf, strlen(sendBuf), 0);
				}

				else
				{
					bool isUserExisted = false;
					//check if the the id exist, and send to that id
					for (int i = 0; i < numClients; i++)
					{

						if (strcmp(add, ids[i]) == 0) // id nhan dc trung voi ids trong mang
						{
							strcpy(sendBuf, "[MESSAGE] ");
							strcat(sendBuf, id);  // cat user id
							strcat(sendBuf, ": ");
							strcat(sendBuf, buf + strlen(cmd) + 1 + strlen(add) + 1); // cat user message
							send(clients[i], sendBuf, strlen(sendBuf), 0);
							isUserExisted = true;
							break;
						}
					}
					if (isUserExisted == false)
					{
						errorMsg = "no user";
						strcpy(sendBuf, "[SEND] ERROR ");
						strcat(sendBuf, errorMsg);
						send(client, sendBuf, strlen(sendBuf), 0);
					}

				}
			}


		}

	}

	// ket thuc server 
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

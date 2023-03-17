//
// CommunicatorClient
//

// Credits: https://www.binarytides.com/udp-socket-programming-in-winsock/

#include <iostream>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

//
// Definitions
//
#define BUF_LEN         ( 512 )
#define SERVER          ("127.0.0.1")
#define PORT            ( 5001 )

//
// Macros
//
#define ZERO_STRUCT(s)	memset( ( char* )&s, '\0', sizeof(s) )
#define ZERO_BUF(arr)	memset( ( char* )arr,  '\0', sizeof(arr) )

using namespace std;

int main(void)
{
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	char buf[BUF_LEN] = { 0 };
	char message[BUF_LEN] = { 0 };
	WSADATA wsa;
	SOCKET s;

	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	//setup address structure
	memset((char*)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER, &si_other.sin_addr.S_un.S_addr);

	//start communication
	while (1)
	{
		// Enter string to be send via the socket
		printf("Enter message : ");
		if (scanf_s("%511s", message, (unsigned int)_countof(message)) != EOF)
		{
			printf("Your entered: %s\n", message);
		}
		else
		{
			printf("scanf_s() failed to get user input.");
			exit(EXIT_FAILURE);
		}

		//send the message
		if (sendto(s, message, (int)strlen(message), 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//receive a reply and print it
		ZERO_BUF(buf);

		//try to receive some data, this is a blocking call
		if (recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&si_other, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		printf("Buffer: %s", buf);
	}

	closesocket(s);
	WSACleanup();

	return 0;
}
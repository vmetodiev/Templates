//
// CommunicatorServer
//

// Credits: https://www.binarytides.com/udp-socket-programming-in-winsock/

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

//
// Definitions
//
#define BUF_LEN         ( 512 )
#define PORT            ( 5001 )
#define IP_ADDR_STR_LEN ( 128 )

#define TERM_WORD       ( "END" )
#define TERM_WORD_LEN   ( 4 )

//
// Macros
//
#define ZERO_STRUCT(s)	memset( ( char* )&s, '\0', sizeof(s) )
#define ZERO_BUF(arr)	memset( ( char* )arr,  '\0', sizeof(arr) )

using namespace std;

// Prototypes
int BlockindUdpServer(void);
int NonBlockindUdpServer(void);

//
// Blocking UDP server implementation
//
int BlockindUdpServer(void)
{
    void* addr_ptr = NULL;
    char ip_str[IP_ADDR_STR_LEN];

    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen, recv_len;
    char buf[BUF_LEN];
    char term[TERM_WORD_LEN] = TERM_WORD;
    WSADATA wsa;

    slen = sizeof(si_other);

    // Initialise winsock v2.2
    printf("\nInitialiase Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised.\n");

    // Create socket
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }
    printf("Socket created.\n");

    // Populate the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Bind successful.\n");

    // Listen for data
    while (1)
    {
        printf("Waiting for data...");
        fflush(stdout);

        // Clear the buffer
        ZERO_BUF(buf);

        // Attemp to receive some data (via blocking call)
        if ((recv_len = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR)
        {
            printf("recvfrom() failed with error code : %d", WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        // Print details of the client/peer and the data received
        printf("Received packet from %s: %d\n",
            inet_ntop(si_other.sin_family, (void*)&si_other.sin_addr, (PSTR)ip_str, sizeof(ip_str)),
            ntohs(si_other.sin_port));

        printf("Received %d bytes, data: %s\n", recv_len, buf);

        // Echo data back to the client
        if (sendto(s, buf, recv_len, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
        {
            printf("sendto() failed with error code : %d", WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        if (TERM_WORD_LEN == recv_len)
        {
            unsigned short term_cnt = 0;
            for (int ptr = 0; ptr < recv_len; ptr++)
            {
                if (buf[ptr] == term[ptr])
                    term_cnt++;
            }

            if (term_cnt == TERM_WORD_LEN - 1)
                break;
        }
    }

    closesocket(s);
    WSACleanup();

    return 0;
}

//
// Non-blocking UDP server implementation
//
int NonBlockindUdpServer(void)
{
    void* addr_ptr = NULL;
    char ip_str[IP_ADDR_STR_LEN];

    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen, recv_len;
    char buf[BUF_LEN];
    char term[TERM_WORD_LEN] = TERM_WORD;
    WSADATA wsa;

    slen = sizeof(si_other);

    // Initialise winsock v2.2
    printf("\nInitialiase Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised.\n");

    // Create socket
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }
    printf("Socket created.\n");

    // Make it non-blocking
    unsigned long ul = 1;
    int non_blocking_ret;

    non_blocking_ret = ioctlsocket(s, FIONBIO, (unsigned long*)&ul);

    if (non_blocking_ret == SOCKET_ERROR)
    {
        printf("Could not set socket as non-blocking : %d", WSAGetLastError());
    }
    printf("Socket set as non-blocking.\n");

    // Populate the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Bind successful.\n");

    // Listen for data
    while (1)
    {
        fflush(stdout);

        // Clear the buffer
        ZERO_BUF(buf);

        recv_len = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&si_other, &slen);

        if (recv_len < 0)
        {
            // No client present, no data - use the CPU time to do something else
            /* printf("."); */
        }
        else
        {
            // Print details of the client/peer and the data received
            printf("Received packet from %s: %d\n",
                inet_ntop(si_other.sin_family, (void*)&si_other.sin_addr, (PSTR)ip_str, sizeof(ip_str)),
                ntohs(si_other.sin_port));

            printf("Received %d byes, data: %s\n", recv_len, buf);

            // Echo data back to the client
            if (sendto(s, buf, recv_len, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR)
            {
                printf("sendto() failed with error code : %d", WSAGetLastError());
                exit(EXIT_FAILURE);
            }

            if (TERM_WORD_LEN == recv_len)
            {
                unsigned short term_cnt = 0;
                for (int ptr = 0; ptr < recv_len; ptr++)
                {
                    if (buf[ptr] == term[ptr])
                        term_cnt++;
                }

                if (term_cnt == TERM_WORD_LEN - 1)
                    break;
            }
        }
    }

    closesocket(s);
    WSACleanup();

    return 0;
}

int main()
{
    cout << "--- Startring the Blocking UDP Server ---" << endl;
    int ret_val = NonBlockindUdpServer();
    cout << "--- Stopping the Blocking UDP Server ---" << endl;
    
    return ret_val;
}

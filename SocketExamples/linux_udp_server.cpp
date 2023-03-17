#include "linux_udp_server.h"
#include <stdio.h>

UdpServer::UdpServer()
{

}

UdpServer::~UdpServer()
{

}

#if ( ( PLATFORM == X86_64 ) || ( PLATFORM == AARCH64 ) )
uint64_t UdpServer::RunStandardNonBlockUdpServer(const char *ip, int port)
#elif ( ( PLATFORM == X86 ) || ( PLATFORM == ARM32 ) )
uint32_t UdpServer::RunStandardNonBlockUdpServer(const char *ip, int port)
#endif
{
	uint8_t buf[INBOUND_BUFFER_SIZE];
	
	memset(buf, 0, INBOUND_BUFFER_SIZE);
		
    int socket_id = 0;
    struct sockaddr_in client, server;
	
    // Create datagram socket

    if ( (socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        // Error
        perror("Error creating the socket.\n");
    }
	printf("Socket is: %d\n", socket_id);
	
    // Zero out the structure and buffer
    memset(&server, 0, sizeof(server));
    memset(&client,0, sizeof(client));

	socklen_t addr_len = sizeof(client);
     
    server.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server.sin_addr);
    server.sin_port = htons(port);

    if ( bind(socket_id, (struct sockaddr *)&server, sizeof(server)) < 0 ) {
        // Error
        perror("Error binding name to the socket.\n");
    }

    fcntl(socket_id, F_SETFL, O_NONBLOCK);

    while( true )
    {
        
		memset(buf, 0, INBOUND_BUFFER_SIZE);
		
		ssize_t bytes_read = 0;
		 
		bytes_read = recvfrom(socket_id, buf, INBOUND_BUFFER_SIZE, 0,
				(struct sockaddr *)&client, &addr_len);
		if ( bytes_read < 0 )
		{
			// No client present, no data - use the CPU time to do something else
		}
		else {

			// Do the protocol handling logic later in this block
            for (ssize_t i = 0; i < bytes_read; i++)
                printf("%c", buf[i]);

            #ifdef ECHO_ENABLED           
				sendto(socket_id, (const char *)buf, INBOUND_BUFFER_SIZE, 0,
						(struct sockaddr *)&client, addr_len);
            #endif
		}

	}

    // Deallocate the socket
    close(socket_id);
    return 0;
}

int main (void)
{
    UdpServer udpServer;
    udpServer.RunStandardNonBlockUdpServer("127.0.0.1", 5505);
    return 0;
}
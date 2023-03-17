//
// Compile with:
// $ g++ tcp_client_heartbeat.cpp -o tcp_client_heartbeat -lpthread
//
// Run with:
// $ ./tcp_client_heartbeat
//

//
// Could also be tested against netcat:
// $ nc -lk 127.0.0.1 5505 
// (send a string from the keyboard that is 2 chars long to trigger HEARTBEAT_SERVER_TO_CLIENT transmisstion - examples "s<Enter>", "sa", "se", etc.)
//

#include <pthread.h>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define INDEX_ZERO ( 0 )
#define INDEX_ONE  ( 1 )
#define HEARTBEAT_PAYLOAD_SIZE_IN_BYTES ( 2 )
#define ZERO_BUF( buffer ) ( memset(buffer, 0, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES ) )

#define TIMER_TICK_INTERVAL_MS ( 500 )
#define MAX_TIMER_TICKS ( 15 )

#define HEARTBEAT_SERVER_TO_CLIENT ( 's' )
#define HEARTBEAT_CLIENT_TO_SERVER ( 'c' )
#define NETCAT_LINE_FEED ( '\n' ) 

//
// Function prototypes
//
void* tcp_client_thread_cb(void* param);
void* timer_thread_cb(void* param);
void tcp_client_send_packet(const char* ip, int port, uint8_t* payload, size_t len, bool await_response);

//
// Global variables accross different threads
//
std::atomic<uint16_t> timer_ticks(0);

//
// Thread argument typedefs
//
typedef struct {
    uint16_t limit;
} timer_thread_args;

typedef struct {
    const char* ip;
    int port;
} tcp_client_thread_args;

//
// Timer thread callback
//
void* timer_thread_cb(void *param)
{
    while ( true ) 
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(TIMER_TICK_INTERVAL_MS) );
        
        if ( timer_ticks >= ((timer_thread_args *)param)->limit )
        {    
            printf("Timeout, entering FAIL SAFE STATE!\n"); // In production will may execute the "Enter Fail Safe State" only once, but here we want intrusive output log.
        }
        else
            timer_ticks++;
    }

    return nullptr;
}

//
// TCP client send packet
//
void tcp_client_send_packet(const char* ip, int port, uint8_t* payload, size_t len, bool await_response)
{
    struct sockaddr_in server;
    int s;
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port        = htons(port);

    if ( ( s = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) // If socket creation fails, goto finalize
    {
        perror("Socket()");
        goto finalize;
    }

    // Set recv() timeout for the blocking client socket - 1.25s should be adequate (ask Ivan or Vladislav about the most optimal value)
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 250000;
    setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv) );

    if ( connect( s, (struct sockaddr *)&server, sizeof(server) ) < 0 ) // If connect fails, goto finalize
    {
        perror("Connect()");
        goto finalize;
    }

    if ( send(s, payload, len, 0) < 0 ) // If send fails, goto finalize
    {
        perror("Send()");
        goto finalize;
    }

	if ( await_response )
	{
        ZERO_BUF( payload );
		ssize_t recv_ret = recv(s, payload, len, 0);
        
        if ( recv_ret == HEARTBEAT_PAYLOAD_SIZE_IN_BYTES ) // Could also be >= condition - depending on the protocol requirements and the other message formats 
        {
            printf("Recv() %lu bytes from server.\n", recv_ret);
            for ( int i = 0; i < recv_ret; i++ )
                printf( "<--- payload[%d] = %c\n", i, payload[i] );
        
            if ( payload[INDEX_ZERO] == HEARTBEAT_SERVER_TO_CLIENT ) // If the server responds with server-to-client heartbeat, reset the timer
            {
                timer_ticks = 0; // Reset the timer
                printf("Recv() HEARTBEAT_SERVER_TO_CLIENT, reseting the timer.\n");
            }
        }
        else
			printf("Recv() could not receive a heartbeat from the server, returned: %ld\n", recv_ret);
        
        goto finalize; // In all cases, proceed to finalize
	}

finalize:
	close(s);
	return;
}

//
// TCP Client thread callback
//
void* tcp_client_thread_cb(void* param)
{   
    uint8_t payload[HEARTBEAT_PAYLOAD_SIZE_IN_BYTES]  = { 0 }; 
    
    while ( true )
    {
        // Send the server-to-client heartbeat
        payload[INDEX_ZERO] = HEARTBEAT_CLIENT_TO_SERVER;
        payload[INDEX_ONE] = NETCAT_LINE_FEED;

        tcp_client_send_packet( ((tcp_client_thread_args *)param)->ip, ((tcp_client_thread_args *)param)->port, payload, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES, true );
        std::this_thread::sleep_for( std::chrono::seconds(1) ); // One seconds delay for comfortable log observation...
    }

    return nullptr;
}

int main (void)
{
    cpu_set_t cpuset;
    
    timer_thread_args timer_thread_args_obj = { .limit = MAX_TIMER_TICKS };
    tcp_client_thread_args tcp_client_thread_args_obj = { .ip = "127.0.0.1", .port = 5505 };

    pthread_t timer_thread;
    pthread_t tcp_client_thread;

    pthread_create( &timer_thread, 0, timer_thread_cb,  (void *)&timer_thread_args_obj );
    pthread_create( &tcp_client_thread, 0, tcp_client_thread_cb,  (void *)&tcp_client_thread_args_obj );

    int s;
    
    CPU_SET(0, &cpuset);
    s = pthread_setaffinity_np(timer_thread, sizeof(cpuset), &cpuset);
    if (s != 0)
        printf("FATAL: Could not set CPU core affinity for the timer thread...\n");

    CPU_SET(0, &cpuset);
    s = pthread_setaffinity_np(tcp_client_thread, sizeof(cpuset), &cpuset);
    if (s != 0)
        printf("FATAL: Could not set CPU core affinity for the tcp client thread...\n");

    pthread_join( timer_thread, nullptr );
    pthread_join( tcp_client_thread, nullptr );
    
    return 0;
}
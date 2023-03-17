//
// Compile with:
// $ g++ tcp_server_heartbeat.cpp -o tcp_server_heartbeat -lpthread
//
// Run with:
// $ ./tcp_server_heartbeat
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

#define INDEX_ZERO                        ( 0 )
#define INDEX_ONE                         ( 1 )
#define HEARTBEAT_PAYLOAD_SIZE_IN_BYTES   ( 2 )
#define ZERO_BUF( buffer )                ( memset( buffer, 0, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES ) )

#define SOCKET_CONTINUOUS_OPERATION       ( 1 ) // Non-continous: the client connects->sends->closes the connection.

#define TIMER_TICK_INTERVAL_MS            ( 500 )
#define MAX_TIMER_TICKS                   ( 15 )

#define HEARTBEAT_SERVER_TO_CLIENT        ( 's' )
#define HEARTBEAT_CLIENT_TO_SERVER        ( 'c' )
#define NETCAT_LINE_FEED                  ( '\n' )

//
// Function prototypes
//
void* tcp_non_blocking_server_cb(void* param);
void* timer_thread_cb(void* param);
void tcp_non_blocking_server(const char* ip, int port, uint8_t* payload, size_t len);

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
} tcp_non_blocking_server_thread_args;

//
// Close socket helper function
//
void close_client_socket(int client_sock_fd)
{
    if ( client_sock_fd > 0 )
    {
        printf("Closing the client socket.\n");
        close(client_sock_fd);
    }
}

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
// TCP Server - should server not only the heartbeat, but everything related to TCP communication of the software component
//
void tcp_non_blocking_server(const char* ip, int port, uint8_t* payload, size_t len)
{
    struct sockaddr_in server;
    socklen_t addr_len     = sizeof(struct sockaddr);;
    int sock_fd, rc, on    = 1;
    int client_sock_fd     = 1;
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port        = htons(port);
    
    if ( ( sock_fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0  ) 
    {
        perror("Socket()");
        goto finalize;
    }

    rc = setsockopt( sock_fd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on) );
    if ( rc < 0 )
    {
        perror("SetSockOpt() failed upon trying to set SO_REUSEADDR.");
        goto finalize;
    }

    rc = fcntl(sock_fd, F_SETFL, O_NONBLOCK );
    if ( rc < 0 )
    {
        perror("SetSockOpt() failed upon trying to set O_NONBLOCK.");
        goto finalize;
    }


    if ( bind( sock_fd, (struct sockaddr *)&server, addr_len ) < 0 )
    {
        perror("Bind()");
        goto finalize;
    }

    if ( listen( sock_fd, 1 ) < 0 ) // listen( sock_fd, 0 ) would be a bit tricky and undefined, that is why we explicitly set 1
    {
        perror("Listen()");
        goto finalize;
    }

    while ( true )
    {
        client_sock_fd = accept( sock_fd, (struct sockaddr *)&server, &addr_len );
        
        if ( client_sock_fd == -1 )
        {
            if ( ( errno == EWOULDBLOCK ) || ( errno == EAGAIN ) ) // No client has connected
            {
                close_client_socket(client_sock_fd);
            }
        }
        
        else if ( client_sock_fd > 0 ) // client connected
        {
            printf("Client connected...\n");
            
            #if ( SOCKET_CONTINUOUS_OPERATION )
            while ( true )
            {
            #endif
                ZERO_BUF( payload );
                size_t recv_ret = recv( client_sock_fd, payload, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES, 0 );

                if ( recv_ret > 0 ) // Could also be some other condition - depending on the protocol requirements and the other message formats 
                {
                    for ( int i = 0; i < recv_ret; i++ )
                        printf( "<--- payload[%d] = %c\n", i, payload[i] );

                    if ( payload[INDEX_ZERO] == HEARTBEAT_CLIENT_TO_SERVER ) // If the client sends client-to-server heartbeat, reset the timer
                    {
                        timer_ticks = 0; // Reset the timer
                        printf("Recv() HEARTBEAT_CLIENT_TO_SERVER, reseting the timer.\n");

                        // And respond with a server-to-client-heartbeat;
                        payload[INDEX_ZERO] = HEARTBEAT_SERVER_TO_CLIENT;
                        payload[INDEX_ONE] = NETCAT_LINE_FEED;
                        send( client_sock_fd, payload, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES, 0 );
                    }
                }
                
                else // Client could have disconnected or Recv() received less than the expected bytes...
                {
                    #if ( SOCKET_CONTINUOUS_OPERATION )
                    close_client_socket(client_sock_fd);
                    break;
                    #endif
                }

                #if ( !SOCKET_CONTINUOUS_OPERATION )
                    close_client_socket(client_sock_fd);
                #endif

            #if ( SOCKET_CONTINUOUS_OPERATION )
            }
            #endif
        }
    }

finalize:
    close(sock_fd);
	return;
}

//
// TCP Server thread callback
//
void* tcp_non_blocking_server_thread_cb(void* param)
{   
    uint8_t payload[HEARTBEAT_PAYLOAD_SIZE_IN_BYTES]  = { 0 }; 
         
    tcp_non_blocking_server("127.0.0.1", 5505, payload, HEARTBEAT_PAYLOAD_SIZE_IN_BYTES);

    return nullptr;
}

int main(void)
{
    cpu_set_t cpuset;
    
    timer_thread_args timer_thread_args_obj = { .limit = MAX_TIMER_TICKS };
    tcp_non_blocking_server_thread_args tcp_non_blocking_server_thread_args_obj = { .ip = "127.0.0.1", .port = 5505 };

    pthread_t timer_thread;
    pthread_t tcp_non_blocking_server_thread;

    pthread_create( &timer_thread, 0, timer_thread_cb,  (void *)&timer_thread_args_obj );
    pthread_create( &tcp_non_blocking_server_thread, 0, tcp_non_blocking_server_thread_cb,  (void *)&tcp_non_blocking_server_thread_args_obj );

    int s;
    
    CPU_SET(0, &cpuset);
    s = pthread_setaffinity_np(timer_thread, sizeof(cpuset), &cpuset);
    if (s != 0)
        printf("FATAL: Could not set CPU core affinity for the timer thread...\n");

    CPU_SET(0, &cpuset);
    s = pthread_setaffinity_np(tcp_non_blocking_server_thread, sizeof(cpuset), &cpuset);
    if (s != 0)
        printf("FATAL: Could not set CPU core affinity for the tcp client thread...\n");

    pthread_join( timer_thread, nullptr );
    pthread_join( tcp_non_blocking_server_thread, nullptr );
    
    return 0;
}
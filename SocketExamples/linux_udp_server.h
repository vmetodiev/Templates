#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#pragma once

#define PLATFORM (X86_64)
#define INBOUND_BUFFER_SIZE 4096

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

class UdpServer {
    private:
        void (*ParsePayload)(uint8_t *buffer, int len) = nullptr;

    public:
        UdpServer();
        ~UdpServer();

        #if ( ( PLATFORM == X86_64 ) || ( PLATFORM == AARCH64 ) )
        uint64_t RunStandardNonBlockUdpServer(const char *ip, int port);
        #elif ( ( PLATFORM == X86 ) || ( PLATFORM == ARM32 ) )
        uint32_t RunStandardNonBlockUdpServer(const char *ip, int port);
        #endif
};

#endif
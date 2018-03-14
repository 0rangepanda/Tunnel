#ifndef _PROXY_H_
#define _PROXY_H_

#include "common.h"
#include "utils.h"
#include "parser.h"

/*------------------------------ Threads --------------------------------------*/
extern void* Proxy(void* arg);
extern void* Monitor(void* arg);

/*----------------------------- Proxy Functions ------------------------------*/
class ProxyClass
{
    public:
        ProxyClass(int stage, int sock);
        int stage1();
        FILE* startLog();
        int tunAlloc();
        int readFromRouter();
        int readFromTunnel();

        int showRouterIP();

    protected:

    private:
        int stage;
        int sock;     // to router socket
        int tun_fd;   // tunnel socket
        FILE *logfd;  // log file pointer

        struct sockaddr_in* routerAddr;  // router address
        int nSize;
        socklen_t addrlen;
};

#endif

#include "proxy.h"

/**************************************************************************
* proxy constructor
**************************************************************************/
ProxyClass::ProxyClass(int Istage, int Isock) {
        stage = Istage;
        sock = Isock;

        routerAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        nSize = sizeof(struct sockaddr);
        addrlen = sizeof(struct sockaddr_in);
};

/**************************************************************************
* Init log file for proxy
**************************************************************************/
FILE* ProxyClass::startLog() {
        char filename[MAXPATHLENGTH];
        sprintf(filename, "stage%d.proxy.out", stage);
        logfd = fopen(filename, "w+");
        //logfd = stdout;
        LOG(logfd, "proxy port: %d\n", udp_port);
        return logfd;
};

/**************************************************************************
* proxy stage 1:
* Recieve the setup packet from router 1
* the packet contains pid of the router
* get router's udp port from routerAddr
**************************************************************************/
int ProxyClass::stage1() {
        char buffer[BUF_SIZE];
        int strLen = recvfrom(sock, buffer, BUF_SIZE, 0,
                              (struct sockaddr *)routerAddr, (socklen_t *)&nSize);
        printf("\nRead a packet from router, packet length:%d\n", strLen);
        int router_pid = atoi(buffer);
        LOG(logfd, "router: %d, pid: %d, port: %d\n", 1, router_pid,
            routerAddr->sin_port);
};

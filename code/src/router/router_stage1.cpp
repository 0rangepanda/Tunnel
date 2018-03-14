#include "router.h"

/**************************************************************************
* RouterClass constructor
**************************************************************************/
RouterClass::RouterClass(int Istage, int num, struct sockaddr_in* addr) {
        stage = Istage;
        id = num;

        selfAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        proxyAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        nSize = sizeof(struct sockaddr);
        addrlen = sizeof(struct sockaddr_in);

        memset(proxyAddr, 0, sizeof(struct sockaddr_in));
        proxyAddr->sin_family = AF_INET;
        proxyAddr->sin_addr.s_addr = addr->sin_addr.s_addr;
        proxyAddr->sin_port = addr->sin_port;
};

/**************************************************************************
* Init the log file
**************************************************************************/
FILE* RouterClass::startLog() {
        char filename[MAXPATHLENGTH];
        if (stage<3)
        {
                sprintf(filename, "stage%d.router.out", stage);
        }
        else
        {
                sprintf(filename, "stage%d.router%d.out", stage, id+1);
        }
        logfd = fopen(filename, "w+");
        return logfd;
};

/**************************************************************************
* Send to proxy a hello message
**************************************************************************/
int RouterClass::stage1() {
        /* bind a socket and get a dynamic UDP port*/
        sock = UDP_alloc(selfAddr);
        LOG(logfd, "router: %d, pid: %d, port: %d\n", id, getpid(), selfAddr->sin_port);

        /* send port to Proxy*/
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d", getpid());
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*) proxyAddr, addrlen);

        return sock;
};

int RouterClass::showIP(){
        printf("Router IP address: %s\n", inet_ntoa(selfAddr->sin_addr));
}

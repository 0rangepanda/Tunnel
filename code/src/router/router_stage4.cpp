#include "router.h"


/**************************************************************************
* Send to proxy a hello message
* [pid]+[router_id]
**************************************************************************/
int RouterClass::stage4() {
        /* bind a socket and get a dynamic UDP port*/
        sock = UDP_alloc(selfAddr);
        //LOG(logfd, "router: %d, pid: %d, port: %d\n", id, getpid(), selfAddr->sin_port);
        if (stage > 4)
                LOG(logfd, "router: %d, pid: %d, port: %d, IP:%s\n",
                    id+1, getpid(), selfAddr->sin_port, inet_ntoa(eth[id].sin_addr));
        else
                LOG(logfd, "router: %d, pid: %d, port: %d\n",
                    id+1, getpid(), selfAddr->sin_port);

        /* send port to Proxy*/
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        sprintf(buffer, "%d%d", id, getpid());//id is a number between 1 and 6
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*) proxyAddr, addrlen);

        return sock;
};

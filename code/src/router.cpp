#include "router.h"

/******************************************************************
    When using this Cleanup(), child proc will not be terminated
    by signal but itself, then WIFSIGNALED(status) will return false
*******************************************************************/
void Cleanup(int sig_num){
   LOG(logfd, "Clean up router process!\n");
   fflush(logfd);

   close(sock);
   fclose(logfd);
   exit(1);
}

void Router() {
    signal(SIGTERM, Cleanup);
    signal(SIGABRT, Cleanup);

    char filename[MAXPATHLENGTH];
    sprintf(filename, "stage%d.router.out", stage);
    logfd = fopen(filename, "w+");

    char buffer[BUF_SIZE], str[BUF_SIZE];

    /* bind a socket and get a dynamic UDP port*/
    sock = UDP_alloc(&addr);
    LOG(logfd, "router: %d, pid: %d, port: %d\n", num_routers, getpid(), addr.sin_port);

    /* send port to Proxy*/
    struct sockaddr_in proxyAddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    memset(&proxyAddr, 0, sizeof(struct sockaddr_in));
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = addr.sin_addr.s_addr;
    proxyAddr.sin_port = udp_port;

    memset(&buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", getpid());
    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*) &proxyAddr, addrlen);

    /* Recieve from Proxy*/
    if (stage==2)
    {
        while (1)
        {
            memset(&buffer, 0, sizeof(buffer));
            int nSize = sizeof(struct sockaddr);
            int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) &proxyAddr, (socklen_t*) &nSize);

            printf("\nRouter: Read a packet from proxy, packet length:%d\n", strLen);

            //get an ICMP ECHO packet from Proxy
            Packet *p = new Packet(buffer, strLen);

            if (p->parse())
            {
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n", proxyAddr.sin_port, p->src.data(), p->dst.data(), p->type);
                //send it to the router
                p->icmpReply();
                //p->printPacket();
                p->sendUDP(&proxyAddr, sock, p->getPacket(), p->getPacketLen());
            }
            else
                fprintf(stderr, "Invalid packet!\n");

            delete p;
        }
    }

    close(sock);
    fclose(logfd);
    return;
}

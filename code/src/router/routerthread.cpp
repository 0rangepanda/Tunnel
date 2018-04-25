#include "router.h"

/******************************************************************
 * When using this Cleanup(), child proc will not be terminated
 * by signal but itself, then WIFSIGNALED(status) will return false
 *******************************************************************/
void Cleanup(int sig_num){
        //LOG(logfd, "Clean up router process!\n");
        fflush(logfd);

        close(sock);
        close(raw_sock);
        close(tcp_sock);
        fclose(logfd);
        exit(1);
}

/**************************************************************
 * Router thread
 ***************************************************************/
void Router(int router_id) {
        signal(SIGTERM, Cleanup);
        signal(SIGABRT, Cleanup);

        RouterClass *router = new RouterClass(stage, router_id, &addr);
        logfd = router->startLog();

        if (stage<4)
                sock = router->stage1();
        else
                sock = router->stage4();

        sleep(1);

        if (stage>1)
        {

                fd_set readset;
                int ret;

                if (stage>=3) // for stage 3 and above
                        raw_sock = router->rawAlloc();
                else         // for stage 1 and 2
                        raw_sock = 0;

                int maxfd = (raw_sock>sock) ? (raw_sock) : (sock);

                if (stage>=7) // TCP, for stage 7 and above
                {
                        tcp_sock = router->tcp_rawAlloc();
                        maxfd = (tcp_sock>maxfd) ? (tcp_sock) : (maxfd);
                }
                else
                        tcp_sock = 0;

                //DEBUG("raw_sock: %d", raw_sock);
                //DEBUG("tcp_sock: %d", tcp_sock);

                while (1)
                {

                        FD_ZERO(&readset);
                        FD_SET(raw_sock, &readset);
                        FD_SET(sock, &readset);
                        FD_SET(tcp_sock, &readset);

                        if (stage==9 && router->timer_start==1)
                        {
                                router->setTimeout();
                                ret = select(maxfd+1, &readset, NULL, NULL, router->getTimeout());
                        }
                        else
                                ret = select(maxfd+1, &readset, NULL, NULL, NULL);

                        //pthread_mutex_lock(&mutex);
                        switch (ret)
                        {
                        case -1:
                                perror("select");

                        case  0:
                                printf("time out\n");
                                if(stage==9)
                                        router->sendWorryMsg();

                        default:
                                if(FD_ISSET(sock, &readset))
                                {
                                        readFromProxy(router);
                                }
                                if(FD_ISSET(raw_sock, &readset))
                                {
                                        readFromRaw(router);
                                }
                                if(FD_ISSET(tcp_sock, &readset))
                                {
                                        readFromTCP(router);
                                }
                        }
                        //pthread_mutex_unlock(&mutex);
                        //fflush(logfd);
                }
        }

        close(sock);
        close(raw_sock);
        close(tcp_sock);
        fclose(logfd);
        return;
}

/**************************************************************
 * Router: read a packet from proxy
 ****************************************************************/
int readFromProxy(RouterClass *router)
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        struct sockaddr_in* proxyAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        int nSize = sizeof(struct sockaddr);
        int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) proxyAddr, (socklen_t*) &nSize);
        printf("\nRouter%d: Read a packet from proxy, packet length:%d\n", router->getId()+1, strLen);

        Packet *p = new Packet(buffer, strLen);
        p->parse();

        if (p->type==1)
        {
                //ICMP
                router->handle_ICMPFromProxy(p);
        }
        else if (p->type==253)
        {
                //Mantitor msg
                if (stage==5)
                        router->handle_Ctlmsg_5(p, proxyAddr);
                if (stage>5)
                        router->handle_Ctlmsg_6(p, proxyAddr);
        }

        delete p;
        return 1;
}

/**************************************************************
 * Router: read a packet from raw socket
 ****************************************************************/
int readFromRaw(RouterClass *router)
{
        // read raw socket
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        int buflen = read(raw_sock,buffer,BUF_SIZE);

        if(buflen < 0)
        {
                perror("Reading from raw socket");
                close(raw_sock);
                exit(1);
        }
        else
        {
                printf("\nRouter%d: Read a packet from rawsock, packet length:%d\n", router->getId()+1, buflen);

                Packet* p = new Packet(buffer, buflen);
                p->parse();
                //printIPhdr(buffer, buflen);

                if (p->type==1)
                {
                        //ICMP
                        if (stage<=4) router->handle_ICMPFromRaw(p);
                        if (stage==5) router->handle_ICMPFromRaw_5(p);
                        if (stage==6 || stage==7 || stage==9) router->handle_ICMPFromRaw_6(p);
                        if (stage==8) router->handle_ICMPFromRaw_8(p);
                }
                delete p;
        }

        return 1;
}

/**************************************************************
 * Router: read a packet from TCP raw socket
 ****************************************************************/
int readFromTCP(RouterClass *router){
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        int buflen = read(tcp_sock,buffer,BUF_SIZE);
        if(buflen < 0)
        {
                perror("Reading from TCP raw socket");
                close(raw_sock);
                exit(1);
        }
        else
        {
                printf("\nRouter%d: Read a packet from TCP rawsock, packet length:%d\n", router->getId()+1, buflen);

                Packet* p = new Packet(buffer, buflen);
                p->parse();
                //printTCP(p->getPacket(), p->getPacketLen());

                if (p->type==6)
                {
                        //TCP
                        if (stage==7 ) router->handle_TCPFromRaw(p);
                        if (stage==8 || stage==9) router->handle_TCPFromRaw_8(p);
                }
                delete p;
        }

        return 1;
}

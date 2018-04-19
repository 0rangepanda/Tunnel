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

        router->showIP();

        if (stage>1)
        {

                fd_set readset;
                int ret;

                if (stage>2) // for stage 3 and above
                {
                        raw_sock = router->rawAlloc();
                }
                else         // for stage 1 and 2
                {
                        raw_sock = 0;
                }

                int maxfd = (raw_sock>sock) ? (raw_sock+1) : (sock+1);

                while (1)
                {
                        FD_ZERO(&readset);
                        FD_SET(raw_sock, &readset);
                        FD_SET(sock, &readset);

                        ret = select(maxfd+1, &readset, NULL, NULL, NULL);

                        pthread_mutex_lock(&mutex);
                        switch (ret)
                        {
                        case -1: perror("select");
                        case  0: printf("time out\n");
                        default:
                                if(FD_ISSET(sock, &readset))
                                {
                                        readFromProxy(router);
                                }
                                if(FD_ISSET(raw_sock, &readset))
                                {
                                        readFromRaw(router);
                                }
                        }
                        pthread_mutex_unlock(&mutex);
                        fflush(logfd);
                }
        }

        close(sock);
        close(raw_sock);
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
        else if (p->type==6)
        {
                //TCP
                /* code */
        }
        else if (p->type==253)
        {
                //Mantitor msg
                if (stage==5) router->handle_Ctlmsg_5(p, proxyAddr);
                if (stage==6) router->handle_Ctlmsg_6(p, proxyAddr);
        }

        delete p;
        return 1;
}

/**************************************************************
 * Router: read a packet from raw socket
 ****************************************************************/
int readFromRaw(RouterClass *router)
{
        /* Use recvfrom()

           char *buffer = (char*)malloc(65536);
           memset(buffer,0,65536);
           struct sockaddr_in saddr;
           int saddr_len = sizeof(saddr);

           int buflen=recvfrom(raw_socket,buffer,65536,0,(struct sockaddr*)&saddr,
                            (socklen_t *)&saddr_len);

           if(buflen<0)
                return -1;
         */
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

                if (p->type==1)
                {
                        //ICMP
                        if (stage<=4) router->handle_ICMPFromRaw(p);
                        if (stage==5) router->handle_ICMPFromRaw_5(p);
                        if (stage==6) router->handle_ICMPFromRaw_6(p);
                }
                else if (p->type==6)
                {
                        //TCP
                        /* code */
                }

                delete p;
        }

        return 1;
}

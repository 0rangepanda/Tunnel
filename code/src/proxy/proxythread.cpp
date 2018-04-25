#include "proxy.h"

/**************************************************************
 * Monitor thread: For catching ctrl+c and cleanup
 * NOTE: after stage4, there will be multiple routers to be terminated
 ***************************************************************/
void* Monitor(void* arg){
        int sig;

        sigwait(&signal_set, &sig);
        pthread_mutex_lock(&mutex);
        /*---------------------Clean up----------------------*/
        printf("\nCatch ctrl+c, cleanning up...\n");

        close(tun_fd);
        /* kill router process */
        if (stage<4)
        {
                int status;
                kill(router_pid, SIGTERM);
                wait(&status);
                if (WIFSIGNALED(status)) //if child proc terminate because of signal
                        printf("Child process received singal %d\n", WTERMSIG(status));
        }
        else
        {
                for (int i = 0; i < num_routers; ++i)
                {
                        int status;
                        kill(routers_pid[i], SIGTERM);
                        wait(&status);
                        if (WIFSIGNALED(status)) //if child proc terminate because of signal
                                printf("Child process %d received singal %d\n",
                                       routers_pid[i], WTERMSIG(status));
                }
        }

        fflush(logfd);
        close(sock);
        fclose(logfd);
        pthread_cancel(proxy_thread);
        /*--------------------------------------------------*/
        pthread_mutex_unlock(&mutex);
}


/**************************************************************
 * Proxy thread
 ****************************************************************/
void* Proxy(void* arg)
{
        /* Setting up */
        ProxyClass *proxy = new ProxyClass(stage, sock);
        logfd = proxy->startLog();


        if (stage<4)
                proxy->stage1();
        else
                proxy->stage4();

        proxy->showRouterIP();

        /* Build Circuit */
        sleep(1);
        if (stage==5)
                proxy->buildCirc(minitor_hops);
        else if (stage==6 || stage==7 || stage==9)
                proxy->enc_buildCirc(minitor_hops);
        //NOTE: stage 8 and above will build circ on demand

        if (stage==9)
                proxy->setStage9();

        printDebugLine("All Router Up! Proxy is ready!", 64);


        /* Main loop */
        if (stage>1)
        {
                tun_fd = proxy->tunAlloc();
                //DEBUG("tun_fd: %d", tun_fd);

                fd_set readset;
                int ret;
                int maxfd = (tun_fd>sock) ? (tun_fd) : (sock);
                //DEBUG("MAXFD: %d", maxfd);


                while (1)
                {
                        FD_ZERO(&readset);
                        FD_SET(tun_fd, &readset);
                        FD_SET(sock, &readset);

                        ret = select(maxfd+1, &readset, NULL, NULL, NULL);

                        //pthread_mutex_lock(&mutex);
                        switch (ret)
                        {
                        case -1: perror("select");
                        case  0: printf("time out\n");
                        default:
                                if(FD_ISSET(sock, &readset))
                                {
                                        readFromRouter(proxy);
                                }
                                if(FD_ISSET(tun_fd, &readset))
                                {
                                        readFromTunnel(proxy);
                                }
                        }

                        //For multi circ
                        //check queue and check if circ ready

                        //pthread_mutex_unlock(&mutex);
                        fflush(logfd);
                }
        }

        close(tun_fd);
        close(sock);
        fclose(logfd);
}

/**************************************************************
 * Proxy: read a packet from router
 ****************************************************************/
int readFromRouter(ProxyClass *proxy)
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        struct sockaddr_in* routerAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        int nSize = sizeof(struct sockaddr);
        int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) routerAddr, (socklen_t*) &nSize);

        printDebugLine("Recv from Router!", 64);
        printf("\nProxy: Read a packet from Router, packet length:%d\n", strLen);

        Packet *p = new Packet(buffer, strLen);
        p->parse();

        //NOTE: will not see protocol=6 here
        if (p->type==1)
        {
                //ICMP
                proxy->handle_ICMPFromRouter(p);
        }
        else if (p->type==253)
        {
                //Mantitor msg
                if (stage==5) proxy->handle_Ctlmsg_5(p, routerAddr);
                if (stage==6) proxy->handle_Ctlmsg_6(p, routerAddr);
                if (stage==7) proxy->handle_Ctlmsg_7(p, routerAddr);
                if (stage>=8) proxy->handle_Ctlmsg_8(p, routerAddr);
        }

        //printf("\n------------------------------END------------------------------------\n");
        //printf("*********************************************************************\n");
        return 1;
}

/**************************************************************
 * Proxy: read a packet from tunnel
 ****************************************************************/
int readFromTunnel(ProxyClass *proxy)
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        int nread = read(tun_fd,buffer,BUF_SIZE);

        if(nread < 0)
        {
                perror("Reading from tunnel interface");
                close(tun_fd);
                exit(1);
        }
        else
        {
                //printf("*********************************************************************\n");
                //printf("------------------------------START----------------------------------\n");
                printf("\nProxy: Read a packet from tunnel, packet length:%d\n", nread);

                Packet *p = new Packet(buffer, nread);
                p->parse();
                //printIPhdr(p->getPacket(),p->getPacketLen());


                //printTCP(p->getPacket(), p->getPacketLen());
                if (stage==8)
                {
                        /* handle flow here */
                        p->pkt2flow();
                        if (!proxy->handleFlow(p)) // a new flow or the circuit not done
                                //add the packet in queue
                                return 1;
                }
                // after this, proxy know which circuit is related to this packet


                if (stage==9)
                {
                        proxy->rebuildCirc_9();//block here
                }


                printDebugLine("Send to Router!", 64);
                if (p->type==1)
                {
                        //ICMP
                        proxy->handle_ICMPFromTunnel(p);
                }
                else if (p->type==6)
                {
                        //TCP
                        if (stage>=7) proxy->handle_TCPFromTunnel(p);
                }
                else
                        DEBUG("Unknown proto!");

                if (stage==9)
                        proxy->simuFailure(p);

        }



        return 1;
}

#include "proxy.h"

/**************************************************************
    Monitor thread: For catching ctrl+c and cleanup
***************************************************************/
void* Monitor(void* arg){
    int sig;

    sigwait(&signal_set, &sig);
    pthread_mutex_lock(&mutex);
    /*---------------------Clean up----------------------*/
    printf("\nCatch ctrl+c, cleanning up...\n");

    close(tun_fd);
    /* kill router process */
    int status;
    kill(router_pid, SIGTERM);
    wait(&status);
    if (WIFSIGNALED(status))//if child proc terminate because of signal
        printf("Child process received singal %d\n", WTERMSIG(status));

    fflush(logfd);
    close(sock);
    fclose(logfd);
    pthread_cancel(proxy_thread);
    /*--------------------------------------------------*/
    pthread_mutex_unlock(&mutex);
}

/**************************************************************
    Proxy thread
***************************************************************/
void* Proxy(void* arg)
{
    /* Setting up */
    char buffer[BUF_SIZE];
    unsigned char rbuf[BUF_SIZE];
    int strLen;

    char filename[MAXPATHLENGTH];
    sprintf(filename, "stage%d.proxy.out", stage);
    logfd = fopen(filename, "w+");
    LOG(logfd, "proxy port: %d\n", udp_port);

    struct sockaddr_in routerAddr;
    int nSize = sizeof(struct sockaddr);
    socklen_t addrlen = sizeof(struct sockaddr_in);

    /* Stage 1 */
    strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) &routerAddr, (socklen_t*) &nSize);

    pthread_mutex_lock(&mutex);
    printf("\nRead a packet from router, packet length:%d\n", strLen);
    router_pid = atoi(buffer);
    LOG(logfd, "router: %d, pid: %d, port: %d\n", num_routers, router_pid, routerAddr.sin_port);
    pthread_mutex_unlock(&mutex);
    /* If stage 1 not finish, will block here */

    if (stage==2)
    {
        /* Set up for Connecting to the tunnel interface */
        char tun_name[IFNAMSIZ];
        tun_fd = tun_alloc(strdup("tun1"), IFF_TUN | IFF_NO_PI);
        if(tun_fd < 0)
        {
        	perror("Open tunnel interface");
        	exit(1);
        }

        fd_set readset;
        int ret;
        int maxfd = (tun_fd>sock)?(tun_fd+1):(sock+1);

        while (1)
        {
            FD_ZERO(&readset);
            FD_SET(tun_fd, &readset);
            FD_SET(sock, &readset);

            ret = select(maxfd+1, &readset, NULL, NULL, NULL);

            pthread_mutex_lock(&mutex);
            memset(&buffer, 0, sizeof(buffer));
            switch (ret)
            {
                case -1: perror("select");
                case  0: printf("time out/n");
                default:
                    if(FD_ISSET(sock, &readset))
                    {
                        strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) &routerAddr, (socklen_t*) &nSize);
                        printf("\nProxy: Read a packet from tunnel, packet length:%d\n", strLen);

                        Packet *p = new Packet(buffer, strLen);
                        if (p->parse())
                        {
                            LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n", routerAddr.sin_port, p->src.data(), p->dst.data(), p->type);
                            p->printPacket();
                            //send it to the tunnel
                            write(tun_fd, p->getPacket(), p->getPacketLen());
                        }
                        else
                            fprintf(stderr, "Invalid packet!\n");
                        delete p;

                    }
                    if(FD_ISSET(tun_fd, &readset))
                    {
                        int nread = read(tun_fd,buffer,BUF_SIZE);

                        if(nread < 0)
                        {
                            perror("Reading from tunnel interface");
                            close(tun_fd);
                            exit(1);
                        }
                        else
                        {
                            printf("\nProxy: Read a packet from tunnel, packet length:%d\n", nread);
                            //get an ICMP ECHO packet from tunnel interface
                            Packet *p = new Packet(buffer, nread);
                            if (p->parse())
                            {
                                LOG(logfd, "ICMP from tunnel, src: %s, dst: %s, type: %d\n", p->src.data(), p->dst.data(), p->type);
                                //send it to the router
                                p->sendUDP(&routerAddr, sock, p->getPacket(), p->getPacketLen());
                            }
                            else
                                fprintf(stderr, "Invalid packet!\n");
                            delete p;
                        }
                    }
            }
            pthread_mutex_unlock(&mutex);
        }
        close(tun_fd);
    }

    close(sock);
    fclose(logfd);
}

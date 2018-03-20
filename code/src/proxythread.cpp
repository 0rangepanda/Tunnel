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
        else if (stage==6)
                proxy->enc_buildCirc(minitor_hops);

        /* Main loop */
        if (stage>1)
        {
                tun_fd = proxy->tunAlloc();

                fd_set readset;
                int ret;
                int maxfd = (tun_fd>sock) ? (tun_fd+1) : (sock+1);

                while (1)
                {
                        FD_ZERO(&readset);
                        FD_SET(tun_fd, &readset);
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
                                        if (stage<5)
                                                proxy->readFromRouter();
                                        else if (stage==5)
                                                proxy->readFromRouter_5();
                                        else if (stage==6)
                                                proxy->readFromRouter_6();

                                }
                                if(FD_ISSET(tun_fd, &readset))
                                {
                                        proxy->readFromTunnel();
                                }
                        }
                        pthread_mutex_unlock(&mutex);
                        fflush(logfd);
                }
        }

        close(tun_fd);
        close(sock);
        fclose(logfd);
}

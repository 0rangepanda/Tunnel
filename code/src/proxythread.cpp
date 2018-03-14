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
        if (WIFSIGNALED(status)) //if child proc terminate because of signal
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
        ProxyClass *proxy = new ProxyClass(stage, sock);
        logfd = proxy->startLog();
        proxy->stage1();

        proxy->showRouterIP();

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
                                        proxy->readFromRouter();
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

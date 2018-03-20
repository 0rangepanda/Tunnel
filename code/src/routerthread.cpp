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
        {
                sock = router->stage1();
        }
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
                                        if (stage<5)
                                                router->readFromProxy();
                                        else if (stage==5)
                                                router->readFromProxy_5();
                                        else if (stage==6)
                                                router->readFromProxy_6();
                                }
                                if(FD_ISSET(raw_sock, &readset))
                                {
                                        if (stage<5)
                                                router->readFromRaw();
                                        else if(stage==5)
                                                router->readFromRaw_5();
                                        else if(stage==6)
                                                router->readFromRaw_6();
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

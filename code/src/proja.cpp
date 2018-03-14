#include "proja.h"

static char FILEPATH[MAXPATHLENGTH];

/* ----------------------- Utility Functions ----------------------- */
static
void Usage()
{
        /* print out usage informaiton, i.e. commandline syntax */
        fprintf(stderr, "usage: %s %s\n", "proja", "[tfile]");
        exit(1);
}


static
void ParseCommandLine(int argc, const char *argv[])
{
        argc--, argv++; /* skip the original argv[0] */
        if (argc <= 0)
        {
                Usage();
        }
        else if (argc ==1)
        {
                /* argv[2] filepath */
                strcpy(FILEPATH, argv[0]);
                if (!checkFile(FILEPATH)) exit(1);
        }
        else
        {
                fprintf(stderr, "Too many args!\n");
                Usage();
        }
}
/* ----------------------- Utils --------------------------- */

/**************************************************************
 * Designate IP addresses for eth
 * Proxy pass these to routers through global variables
 * Each router binds to one of them (according to router id)
 ***************************************************************/
int initEth()
{
      eth = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in)*num_routers);
      for (size_t i = 0; i < num_routers; i++) {
          char ip[MAXPATHLENGTH];
          sprintf(ip, "192.168.20%d.2", i+1);
          memset(&eth[i], 0, sizeof(struct sockaddr_in));
          inet_aton(ip, &eth[i].sin_addr);

          eth[i].sin_family = AF_INET;
          //printf("IP address: %s\n", inet_ntoa(eth[i].sin_addr));
      }
}

/* ----------------------- Process() ----------------------- */

static
void Process()
{
        /* Handling Ctrl+C */
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGINT);
        sigprocmask(SIG_BLOCK, &signal_set, 0);

        /* Parse configuration file */
        parseConf(FILEPATH);

        if (stage >2)
        {
                initEth();
        }

        //create a dynamic (operating-system assigned) UDP port
        //shoud be done befor fork() so that router can get the port by global var
        sock = UDP_alloc(&addr);

        std::cout << "udp_port: " <<  addr.sin_port << "\n";
        udp_port = addr.sin_port;

        //fork then
        for (size_t i = 0; i < num_routers; i++) {
                router_pid = fork();

                if (router_pid < 0)
                {
                        cerr << "Failed to fork" << endl;
                        exit(1);
                }
                else if (router_pid == 0)
                {
                        // Code only executed by child process
                        printf("I am the Router process %d, my process id is %d\n", i+1, getpid());
                        Router(i);
                        return;
                }
        }


        // Code only executed by parent process
        printf("I am the Proxy process, my process id is %d\n",getpid());

        pthread_create(&proxy_thread, NULL, Proxy, NULL);
        pthread_create(&monitor_thread, NULL, Monitor, NULL);
        pthread_join(proxy_thread,0);
        pthread_cancel(monitor_thread);
        printf("Finish cleanning up, program end!\n");
        return;
}

/* ----------------------- main() ----------------------- */
int main(int argc, char const *argv[])
{
        ParseCommandLine(argc, argv);
        Process();
        return 0;
}

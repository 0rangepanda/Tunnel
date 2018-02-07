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
        if (!checkFile(FILEPATH))exit(1);
    }
    else
    {
        fprintf(stderr, "Too many args!\n");
        Usage();
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

    //create a dynamic (operating-system assigned) UDP port
    //shoud be done befor fork() so that router can get the port by global var
    sock = UDP_alloc(&addr);

    std::cout << "udp_port: " <<  addr.sin_port << "\n";
    udp_port = addr.sin_port;

    //fork then
    pid_t fpid;
    fpid=fork();
    router_pid = fpid;

    if (fpid < 0)
    {
        cerr << "Failed to fork" << endl;
        exit(1);
    }
    else if (fpid == 0)
    {
        // Code only executed by child process
        printf("i am the child Router process, my process id is %d\n",getpid());
        Router();
    }
    else {
        // Code only executed by parent process
        printf("i am the parent Proxy process, my process id is %d\n",getpid());

        pthread_create(&proxy_thread, NULL, Proxy, NULL);
        pthread_create(&monitor_thread, NULL, Monitor, NULL);

        pthread_join(proxy_thread,0);
        pthread_cancel(monitor_thread);
        printf("Finish cleanning up, program end!\n");
    }
    return;
}

/* ----------------------- main() ----------------------- */
int main(int argc, char const *argv[])
{
    ParseCommandLine(argc, argv);
    Process();
    return 0;
}

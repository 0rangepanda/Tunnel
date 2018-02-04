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
    parseConf(FILEPATH);
    cout << "stage: " << stage << endl;
    cout << "num_routers: " << num_routers << endl;

    //create a dynamic (operating-system assigned) UDP port
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(0);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    getsockname(sock, (struct sockaddr*)&addr, &addrlen);

    //std::cout << "udp_port: " <<  addr.sin_port << "\n";
    udp_port = addr.sin_port;

    //fork then
    pid_t fpid;
    fpid=fork();

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
        Proxy(sock, &addr);
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

#ifndef _ROUTER_H_
#define _ROUTER_H_

#include <unordered_map>

#include "common.h"
#include "utils.h"
#include "parser.h"


/*------------------------------ Threads --------------------------------------*/
extern void Cleanup(int sig_num);
extern void Router(int router_id);

/*----------------------------- Router Functions ------------------------------*/
class RouterClass
{
public:
        RouterClass(int stage, int num, struct sockaddr_in* addr);
        int stage1();
        FILE* startLog();
        int rawAlloc();

        int showIP();

        int sendtoMe(Packet* p, int socket);
        int record(Packet* p);

        int rewritePkt(Packet* p);
        int readFromProxy();
        int readFromRaw();


protected:

private:
        int stage;
        int id;
        int proxy_udp_port;
        int sock;           // to proxy socket
        int raw_socket;     // raw socket

        FILE *logfd;  // log file pointer

        struct sockaddr_in* selfAddr;   // self address
        struct sockaddr_in* proxyAddr;  // proxy address
        int nSize;
        socklen_t addrlen;

        unordered_map<string, string> addressMap;
};

#endif

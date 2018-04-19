#ifndef _PROXY_H_
#define _PROXY_H_

#include <unordered_map>

#include "common.h"
#include "utils.h"
#include "parser.h"
#include "ctlmsg.h"
#include "aes.h"

/*-------------------------- Global Vars --------------------------------------*/
struct circuit {
        int id;
        int seq;
        int len;
        int* hops;
        char** key;
};


/*----------------------------- Proxy Functions ------------------------------*/
class ProxyClass
{
public:
        ProxyClass(int stage, int sock);

        //stage1
        int stage1();
        FILE* startLog();
        int tunAlloc();

        //stage2
        int handle_ICMPFromRouter(Packet* p);
        int handle_ICMPFromTunnel(Packet* p);
        int showRouterIP();

        //stage4
        int stage4();
        struct sockaddr_in* hashDstIP(string ip);

        //stage5
        int handle_Ctlmsg_5(Packet* p, struct sockaddr_in* routerAddr);
        int buildCirc(int minitor_hops);
        int extCirc(struct circuit* circ, int next_hop);
        int tun2Circ  (struct circuit* circ, char* packet, int len);
        int sendtoCirc(struct circuit* circ, char* packet, int len);
        int recvCirc(struct circuit* circ);

        //stage6
        int handle_Ctlmsg_6(Packet* p, struct sockaddr_in* routerAddr);
        char* enc_genKey(int router_id);
        int enc_buildCirc(int minitor_hops);
        int enc_tun2Circ(struct circuit* circ, char* packet, int len);
        int enc_sendKey(struct circuit* circ, int hop_num);
        int enc_extCirc(struct circuit* circ, int hop_num);
        int enc_sendtoCirc(struct circuit* circ, char* packet, int len);

protected:

private:
        int stage;
        int sock;     // to router socket
        int tun_fd;   // tunnel socket
        FILE *logfd;  // log file pointer

        struct sockaddr_in* routerAddr;  // router address
        int nSize;
        socklen_t addrlen;

        //stage 5
        int* hops;  // for 1 circuit
        struct circuit circ1;

        //stage 6
        unordered_map<int, string> srcMap;   //NOTE: circuit ID (0xIDi) to src IP
};

/*------------------------------ Threads --------------------------------------*/
extern void* Proxy(void* arg);
extern void* Monitor(void* arg);
extern int readFromRouter(ProxyClass *proxy);
extern int readFromTunnel(ProxyClass *proxy);

#endif

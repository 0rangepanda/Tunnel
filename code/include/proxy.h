#ifndef _PROXY_H_
#define _PROXY_H_

#include <unordered_map>
#include <queue>

#include "common.h"
#include "utils.h"
#include "parser.h"
#include "ctlmsg.h"
#include "aes.h"
#include "circuit.h"

/*-------------------------- Data Structure ----------------------------------*/
/*
   struct circuit {
        int id;
        int seq;
        int len;
        int* hops;
        char** key;
   };
 */


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

        //stage7
        int handle_Ctlmsg_7(Packet* p, struct sockaddr_in* routerAddr);
        int handle_TCPFromTunnel(Packet* p);

        //stage8
        int handle_Ctlmsg_8(Packet* p, struct sockaddr_in* routerAddr);
        int handleFlow(Packet* p);
        int buildNewCirc(int minitor_hops, struct circuit* circ);
        int recvRlyData(CtlmsgClass *recv_ctlmsg);
        int recvExtDone(CtlmsgClass *recv_ctlmsg);

        //stage9
        int setStage9();
        int killRouter(int killid);
        void simuFailure(Packet* p);
        int recvRouterWorr(CtlmsgClass *recv_ctlmsg, struct circuit *circ);
        int rebuildCirc(int minitor_hops, struct circuit* circ);
        int rebuildCirc_9();

protected:

private:
        //stage 1-4
        int stage;
        int sock;     // to router socket
        int tun_fd;   // tunnel socket
        FILE *logfd;  // log file pointer

        struct sockaddr_in* routerAddr;  // router address
        int nSize;
        socklen_t addrlen;

        //stage 5
        int* hops;              // for 1 circuit
        struct circuit circ1;   // the only 1 circuit

        //stage 6
        unordered_map<int, string> srcMap;   //NOTE: circuit ID (0xIDi) to src IP

        //stage 8
        int seq;
        vector<struct circuit*> circpool;                     //NOTE: circuit seq to circ mapping
        unordered_map<struct flow, struct circuit*> flowMap;  //NOTE: flow to circuit mapping
        vector<Packet*> pktpool;                              //NOTE: Packet queue

        //stage 9
        int killed = 0;
        int* router_status;
        unordered_map<__u16, int> next_nameMap;  //NOTE: next_name to router_id
        int circ1_down = 0; //should replace by a list of active circs
};

/*------------------------------ Threads --------------------------------------*/
extern void* Proxy(void* arg);
extern void* Monitor(void* arg);
extern int readFromRouter(ProxyClass *proxy);
extern int readFromTunnel(ProxyClass *proxy);

#endif

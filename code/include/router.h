#ifndef _ROUTER_H_
#define _ROUTER_H_

#include <unordered_map>

#include "common.h"
#include "utils.h"
#include "parser.h"
#include "ctlmsg.h"
#include "aes.h"

/*----------------------------- Router Functions ------------------------------*/
class RouterClass
{
public:
        RouterClass(int stage, int num, struct sockaddr_in* addr);
        int getId();
        
        //stage1
        int stage1();
        FILE* startLog();
        int showIP();

        //stage2
        int handle_ICMPFromProxy(Packet* p);
        int sendtoMe(Packet* p, int socket);
        int record(Packet* p);

        //stage3
        int rawAlloc();
        int sendtoRaw(Packet* p);
        int handle_ICMPFromRaw(Packet* p);

        //stage4
        int stage4();

        //stage5
        int handle_ICMPFromRaw_5(Packet* p);
        int handle_Ctlmsg_5(Packet* p, struct sockaddr_in* proxyAddr);
        int relayMsg   (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int recvCtlmsg (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int recvExtCtl (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int recvExtDone(CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int recvRlydata(CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int recvRlyret (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int sendRlyret (Packet* p, __u16 inc_port, int seq);

        //stage6
        int handle_ICMPFromRaw_6(Packet* p);
        int handle_Ctlmsg_6(Packet* p, struct sockaddr_in* proxyAddr);
        int enc_recvFake   (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_relayMsg   (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_recvCtlmsg (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_recvExtCtl (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_recvExtDone(CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_recvRlydata(CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_recvRlyret (CtlmsgClass* recv_ctlmsg, __u16 inc_port);
        int enc_sendRlyret (Packet* p, __u16 inc_port, int seq);

        string selfIP;

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

        unordered_map<int, int>    circMap; //NOTE: circuit ID (0xIDi) to the outgoing circuit ID (0xIDo)
        unordered_map<int, __u16>  portMap; //NOTE: circuit ID (0xIDi) to port
        unordered_map<int, string> ipMap;   //NOTE: circuit ID (0xIDi) to string IP

        unordered_map<int, char*> keyMap;   //NOTE: circuit ID (0xIDi) to session key

};

/*------------------------------ Threads --------------------------------------*/
extern void Cleanup(int sig_num);
extern void Router(int router_id);
extern int readFromProxy(RouterClass *router);
extern int readFromRaw(RouterClass *router);


#endif

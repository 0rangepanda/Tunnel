#ifndef _PARSER_H_
#define _PARSER_H_

#include "common.h"
#include "utils.h"
#include "circuit.h"

/*******************************************************************
* psuedo TCP header
*******************************************************************/
/*
struct psd_tcp {
    struct in_addr src;
    struct in_addr dst;
    unsigned char pad;
    unsigned char proto;
    unsigned short tcp_len;
    struct tcphdr tcp;
};
*/

/*******************************************************************
* Helper funcitons
*******************************************************************/
//extern unsigned short in_cksum(unsigned short *addr, int len);
//extern unsigned short in_cksum_tcp(int src, int dst, unsigned short *addr, int len);

/*******************************************************************
* Packet parser class
*******************************************************************/
class Packet
{
public:
        int type; //protocol
        int src_int, dst_int;
        string src, dst;
        int icmptype;

        Packet(char* buf, int read);

        char* getPacket();      //for whole packet
        int getPacketLen();
        char* getPayload();        //for payload
        int getPayloadLen();

        void parse();
        int icmpReply();
        int changeSrc(string srcIP);
        int changeDst(string dstIP);
        int sendUDP(struct sockaddr_in *addr, int sock, char* payload, int len);

        int printPacket();
        int printICMP();

        // for TCP packet
        void parseTCP();
        void recheckPkt();

        int recheckTCP();
        int recheckTCP2();
        void changeSrcPort(__u16 port);
        void changeDstPort(__u16 port);

        unsigned int srcport;
        unsigned int dstport;
        unsigned long seqno;
        unsigned long ackno;

        // for multi-flow
        struct flow f;
        int pkt2flow();
        int rev_pkt2flow();

private:
        // whole packet
        char* packet;
        int len;
        int iphdr_len;

};


#endif

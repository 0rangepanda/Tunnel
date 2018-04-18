#ifndef _PARSER_H_
#define _PARSER_H_

#include "common.h"
#include "utils.h"


class Packet
{
public:
        int type;
        string src;
        string dst;
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

private:
        // whole packet
        char* packet;
        int len;
        int iphdr_len;
};


#endif

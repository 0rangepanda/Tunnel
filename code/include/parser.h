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

    Packet(char* buf, int read);
    int parse();
    char* getPacket();
    int getPacketLen();
    
    int icmpReply();
    int sendUDP(struct sockaddr_in *addr, int sock, char* payload, int len);

    int printPacket();
    int printICMP();

private:
    char* packet;
    int len;
};


#endif

#include "parser.h"

Packet::Packet(char* buf, int nread)
{
        packet = buf;
        len = nread;

        //payload = buf;
        //pay_len = len - IPV4_OFFSET;
}


/*******************************************************************
* Parse the packet, get type, src and dst
* NOTE: if a ICMP packet type=8, return ture
*******************************************************************/
int Packet::parse()
{
        if (len<IPV4_OFFSET+8) // invalid ICMP packet
                return -1;

        char buf[BUF_SIZE];

        /* type */
        sprintf(&buf[0], "%d", *(packet+IPV4_OFFSET));
        type = atoi(buf);

        /* src */
        std::stringstream ss;
        for(size_t i=0; i<4; i++) {
                memset(&buf, 0, sizeof(buf));
                sprintf(&buf[0], "%d", 0xFF & *(packet+IPV4_OFFSET-8+i));
                ss << buf << '.';
        }
        src = ss.str().substr(0, ss.str().length()-1);

        /* dst */
        ss.str("");
        for(size_t i=0; i<4; i++) {
                memset(&buf, 0, sizeof(buf));
                sprintf(&buf[0], "%d", 0xFF & *(packet+IPV4_OFFSET-4+i));
                ss << buf << '.';
        }
        dst = ss.str().substr(0, ss.str().length()-1);

        //cout << dst << " : " << src <<endl;
        return type==8 || type==0;
}


/*******************************************************************
* Print the ICMP ECHO Header:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Identifier          |        Sequence Number        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Data ...
+-+-+-+-+-
*******************************************************************/
int Packet::printICMP()
{
        char converted[BUF_SIZE];
        for(size_t i=0; i<8; i++) {
                sprintf(&converted[i*2], "%02X", *(packet+i+IPV4_OFFSET));
        }

        for (size_t i = 0; i < 2; ++i)
        {
                if (i==0)
                        printf("Type : Code :  Checksum\n");
                else
                        printf(" Identifier : Sequence #\n");

                printf("%c%c : %c%c : %c%c : %c%c\n",
                       converted[8*i+0],converted[8*i+1],converted[8*i+2],converted[8*i+3],
                       converted[8*i+4],converted[8*i+5],converted[8*i+6],converted[8*i+7]);
        }
        return 0;
}


/*******************************************************************
* Print the Whole packet
*******************************************************************/
int Packet::printPacket() {
        char converted[len*2 + 1];
        for(size_t i=0; i<len; i++) {
                sprintf(&converted[i*2], "%02X", *(packet+i));
        }

        for (size_t i = 0; i < len/8; ++i)
        {
                printf("%c%c : %c%c : %c%c : %c%c\n",
                       converted[8*i+0],converted[8*i+1],converted[8*i+2],converted[8*i+3],
                       converted[8*i+4],converted[8*i+5],converted[8*i+6],converted[8*i+7]);
        }
        return 0;
}


/*******************************************************************
* Generate the ICMP echo reply Packet of char* p
*******************************************************************/
int Packet::icmpReply()
{
        /* type = 0 */
        *(packet+IPV4_OFFSET) = 0;

        /* swap dst and src */
        char tmp;
        for(size_t i=0; i<4; i++) {
                tmp = *(packet+IPV4_OFFSET-8+i);
                *(packet+IPV4_OFFSET-8+i) = *(packet+IPV4_OFFSET-4+i);
                *(packet+IPV4_OFFSET-4+i) = tmp;
        }

        return 0;
}


/*******************************************************************
* change the src of packet
*******************************************************************/
int Packet::changeSrc(string srcIP)
{
        //get src ip address in a 32bit int
        struct sockaddr_in src;
        memset(&src, 0, sizeof(struct sockaddr_in));
        inet_aton(srcIP.c_str(), &src.sin_addr);
        int ip = src.sin_addr.s_addr;

        //change src in place
        for(size_t i=0; i<4; i++)
        {
                *(packet+IPV4_OFFSET-8+i) = (ip >> 8*i) & 0xFF;
        }

        //renew src
        std::stringstream ss;
        char buf[BUF_SIZE];
        for(size_t i=0; i<4; i++) {
                memset(&buf, 0, sizeof(buf));
                sprintf(&buf[0], "%d", 0xFF & *(packet+IPV4_OFFSET-8+i));
                ss << buf << '.';
        }
        this->src = ss.str().substr(0, ss.str().length()-1);

        return 1;
}


/*******************************************************************
* change the dst of packet
*******************************************************************/
int Packet::changeDst(string dstIP)
{
        //get dst ip address in a 32bit int
        struct sockaddr_in dst;
        memset(&dst, 0, sizeof(struct sockaddr_in));
        inet_aton(dstIP.c_str(), &dst.sin_addr);
        int ip =  dst.sin_addr.s_addr;

        //change dst in place
        for(size_t i=0; i<4; i++)
        {
                *(packet+IPV4_OFFSET-4+i) = (ip >> 8*i) & 0xFF;
        }

        //renew dst
        std::stringstream ss;
        char buf[BUF_SIZE];
        for(size_t i=0; i<4; i++) {
                memset(&buf, 0, sizeof(buf));
                sprintf(&buf[0], "%d", 0xFF & *(packet+IPV4_OFFSET-4+i));
                ss << buf << '.';
        }
        this->dst = ss.str().substr(0, ss.str().length()-1);
        return 1;
}

/*******************************************************************
* Get the whole packet (include IPv4 header)
*******************************************************************/
char* Packet::getPacket()
{
        return packet;
}


/*******************************************************************
* Get the length of packet
*******************************************************************/
int Packet::getPacketLen() {
        return len;
}

/*******************************************************************
* Get the packet content (exclude IPv4 header)
*******************************************************************/
char* Packet::getPayload()
{
        return packet+IPV4_OFFSET;
}


/*******************************************************************
* Get the length of packet content (exclude IPv4 header)
*******************************************************************/
int Packet::getPayloadLen() {
        return len-IPV4_OFFSET;
}


/*******************************************************************
* Send the packet payload to another UDP socket
*******************************************************************/
int Packet::sendUDP(struct sockaddr_in *addr, int sock, char* payload, int len){
        socklen_t addrlen = sizeof(struct sockaddr_in);
        sendto(sock, payload, len, 0, (struct sockaddr*) addr, addrlen);
        return 0;
}


/*******************************************************************
* Check if the packet is a ICMP packet
*******************************************************************/
int Packet::ifICMP()
{
        return type==8;
}

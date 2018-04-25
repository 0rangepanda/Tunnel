#include "parser.h"

Packet::Packet(char *buf, int nread) {
        packet = buf;
        len = nread;

        // payload = buf;
        // pay_len = len - IPV4_OFFSET;
}

/*******************************************************************
* Parse the packet, get type, src and dst
* NOTE:
*******************************************************************/
void Packet::parse() {
        struct sockaddr_in source, dest;
        char src_addr_buf[BUF_SIZE];
        char dst_addr_buf[BUF_SIZE];

        struct iphdr *iph = (struct iphdr *)packet;

        this->src_int = iph->saddr;
        this->dst_int = iph->daddr;

        source.sin_addr.s_addr = iph->saddr;
        dest.sin_addr.s_addr = iph->daddr;
        memset(src_addr_buf, 0, BUF_SIZE);
        memset(dst_addr_buf, 0, BUF_SIZE);
        strcpy(src_addr_buf, inet_ntoa(source.sin_addr));
        strcpy(dst_addr_buf, inet_ntoa(dest.sin_addr));

        this->type = iph->protocol;
        this->iphdr_len = iph->ihl * 4;
        this->src = src_addr_buf;
        this->dst = dst_addr_buf;

        this->len = ntohs(iph->tot_len)>0 ? ntohs(iph->tot_len) : len;

        if (this->type == 1)
        {
                // ICMP
                struct icmphdr *icmph = (struct icmphdr *)(packet + iphdr_len);
                this->icmptype = icmph->type;
                this->srcport = 0;
                this->dstport = 0;
        }
        else if (type == 6) {
                // TCP
        }
        else if (type == 253) {
                // the experimental IP protocol number
        }
        return;
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
int Packet::printICMP() {
        char converted[BUF_SIZE];
        for (size_t i = 0; i < 8; i++) {
                sprintf(&converted[i * 2], "%02X", *(packet + i + IPV4_OFFSET));
        }

        for (size_t i = 0; i < 2; ++i) {
                if (i == 0)
                        printf("Type : Code :  Checksum\n");
                else
                        printf(" Identifier : Sequence #\n");

                printf("%c%c : %c%c : %c%c : %c%c\n", converted[8 * i + 0],
                       converted[8 * i + 1], converted[8 * i + 2], converted[8 * i + 3],
                       converted[8 * i + 4], converted[8 * i + 5], converted[8 * i + 6],
                       converted[8 * i + 7]);
        }
        return 0;
}

/*******************************************************************
* Print the Whole packet
*******************************************************************/
int Packet::printPacket() {
        char converted[len * 2 + 1];
        for (size_t i = 0; i < len; i++) {
                sprintf(&converted[i * 2], "%02X", *(packet + i));
        }

        for (size_t i = 0; i < len / 8; ++i) {
                printf("%c%c : %c%c : %c%c : %c%c\n", converted[8 * i + 0],
                       converted[8 * i + 1], converted[8 * i + 2], converted[8 * i + 3],
                       converted[8 * i + 4], converted[8 * i + 5], converted[8 * i + 6],
                       converted[8 * i + 7]);
        }
        return 0;
}

/*******************************************************************
* Generate the ICMP echo reply Packet of [char* p] in place
*******************************************************************/
int Packet::icmpReply() {
        struct iphdr *iph = (struct iphdr *)packet;
        struct icmphdr *icmph = (struct icmphdr *)(packet + iphdr_len);

        // type = 0
        icmph->type = 0;

        // swap dst and src
        struct sockaddr_in tmp;
        tmp.sin_addr.s_addr = iph->saddr;
        iph->saddr = iph->daddr;
        iph->daddr = tmp.sin_addr.s_addr;

        // recompute checksum
        icmph->checksum = 0;
        icmph->checksum = in_cksum((unsigned short *)icmph, sizeof(struct icmphdr));

        iph->check = 0;
        iph->check = in_cksum((unsigned short *)iph, sizeof(struct iphdr));

        return 0;
}

/*******************************************************************
* change the src of packet
* and recompute the checksum of iphdr
*******************************************************************/
int Packet::changeSrc(string srcIP) {
        struct iphdr *iph = (struct iphdr *)packet;

        /// change src in place
        struct sockaddr_in src;
        memset(&src, 0, sizeof(struct sockaddr_in));
        inet_aton(srcIP.c_str(), &src.sin_addr);

        iph->saddr = src.sin_addr.s_addr;

        // recompute checksum
        //iph->check = 0;
        //iph->check = in_cksum((unsigned short *)iph, sizeof(struct iphdr));
        recheckPkt();

        // renew src
        char src_addr_buf[BUF_SIZE];
        memset(src_addr_buf, 0, BUF_SIZE);
        strcpy(src_addr_buf, inet_ntoa(src.sin_addr));
        this->src = src_addr_buf;

        // printf("Debug change src: %s -> %s\n", srcIP.data(), src_addr_buf);
        return 1;
}

/*******************************************************************
* change the dst of packet
* and recompute the checksum of iphdr
*******************************************************************/
int Packet::changeDst(string dstIP) {
        struct iphdr *iph = (struct iphdr *)packet;

        /// change src in place
        struct sockaddr_in dst;
        memset(&dst, 0, sizeof(struct sockaddr_in));
        inet_aton(dstIP.c_str(), &dst.sin_addr);

        iph->daddr = dst.sin_addr.s_addr;

        // recompute checksum
        //iph->check = 0;
        //iph->check = in_cksum((unsigned short *)iph, sizeof(struct iphdr));
        recheckPkt();

        // renew src
        char dst_addr_buf[BUF_SIZE];
        memset(dst_addr_buf, 0, BUF_SIZE);
        strcpy(dst_addr_buf, inet_ntoa(dst.sin_addr));
        this->dst = dst_addr_buf;

        return 1;
}

/*******************************************************************
* Get the whole packet (include IPv4 header)
*******************************************************************/
char *Packet::getPacket() {
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
char *Packet::getPayload() {
        // return packet+IPV4_OFFSET;
        return packet + iphdr_len;
}

/*******************************************************************
* Get the length of packet content (exclude IPv4 header)
*******************************************************************/
int Packet::getPayloadLen() {
        // return len-IPV4_OFFSET;
        return len - iphdr_len;
}

/*******************************************************************
* Send the packet payload to another UDP socket
*******************************************************************/
int Packet::sendUDP(struct sockaddr_in *addr, int sock, char *payload,
                    int len) {
        socklen_t addrlen = sizeof(struct sockaddr_in);
        sendto(sock, payload, len, 0, (struct sockaddr *)addr, addrlen);
        return 0;
}

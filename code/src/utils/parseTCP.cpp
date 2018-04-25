#include "parser.h"

/*******************************************************************
* Parse the TCP packet, get srcport, dstport, seqno, ackno
*******************************************************************/
void Packet::parseTCP() {
        if (this->type!=6) {
                return;
        }
        struct tcphdr *tcph = (struct tcphdr *)(packet+iphdr_len);

        srcport = ntohs(tcph->source);
        dstport = ntohs(tcph->dest);
        seqno   = ntohl(tcph->seq);
        ackno   = ntohl(tcph->ack_seq);
}


/*******************************************************************
* recompute checksum of iphdr
*******************************************************************/
void Packet::recheckPkt(){
        struct iphdr *iph = (struct iphdr *)packet;
        iph->check = 0;
        iph->check = in_cksum((unsigned short *)iph, sizeof(struct iphdr));
}

/*******************************************************************
* change srcport of tcphdr
*******************************************************************/
void Packet::changeSrcPort(__u16 port){
        //printf("Debug port: %d\n", port);
        struct tcphdr *tcph = (struct tcphdr *)(packet+iphdr_len);
        tcph->source = htons(port);
}

/*******************************************************************
* change dstport of tcphdr
*******************************************************************/
void Packet::changeDstPort(__u16 port){
        struct tcphdr *tcph = (struct tcphdr *)(packet+iphdr_len);
        tcph->dest = htons(port);
}

/*******************************************************************
* recompute checksum of tcphdr
*******************************************************************/
int Packet::recheckTCP2(){
        if (this->type!=6) {
                return -1;
        }

        struct iphdr *iph = (struct iphdr *)(packet);
        iphdr_len = iph->ihl * 4;
        struct tcphdr* tcph= (struct tcphdr*)(packet+iphdr_len);

        //recompute checksum of tcp
        tcph->check = 0;

        int tcp_len = ntohs(iph->tot_len) - iphdr_len;


        DEBUG("iph->tot_len: %d\n", ntohs(iph->tot_len));
        DEBUG("tcph->doff: %d\n", tcph->doff);

        if( (unsigned int) tcph->syn == 1)
        {
                tcph->doff = sizeof(struct tcphdr)/4;
                tcp_len = sizeof(struct tcphdr);
        }
        DEBUG("TCP len: %d\n", tcp_len);


        if (tcp_len%2==1)
                tcph->check = in_cksum_tcp(iph->saddr, iph->daddr, (unsigned short *)tcph, tcp_len+1);
        else
                tcph->check = in_cksum_tcp(iph->saddr, iph->daddr, (unsigned short *)tcph, tcp_len);

        return tcp_len;
}

/*******************************************************************
* recompute checksum of tcphdr Version 2
*******************************************************************/
int Packet::recheckTCP(){
        if (this->type!=6) {
                return -1;
        }


        struct iphdr *iph = (struct iphdr *)(packet);
        iphdr_len = iph->ihl * 4;
        struct tcphdr* tcph= (struct tcphdr*)(packet+iphdr_len);


        //recompute checksum of tcp
        tcph->check = 0;
        int tcp_len = ntohs(iph->tot_len) - iphdr_len;
        DEBUG("TCP len: %d\n", tcp_len);

        tcph->check = in_cksum_tcp(iph->saddr, iph->daddr, (unsigned short *)tcph, tcp_len);

        /*
        if (tcp_len%2==1)
                tcph->check = in_cksum_tcp(iph->saddr, iph->daddr, (unsigned short *)tcph, tcp_len+1);
        else
                tcph->check = in_cksum_tcp(iph->saddr, iph->daddr, (unsigned short *)tcph, tcp_len);
        */


        return tcp_len;
}

/**************************************************************************
* Parse a packet, get its flow, store the correlated flow as a member var
**************************************************************************/
int Packet::pkt2flow()
{
        this->parse();
        this->parseTCP();

        f.src =   this->src_int;
        f.dst =   this->dst_int;
        f.proto = this->type;

        f.srcport = (int) this->srcport;
        f.dstport = (int) this->dstport;

        return 1;
}

/**************************************************************************
* For incoming pkt
**************************************************************************/
int Packet::rev_pkt2flow()
{
        this->parse();
        this->parseTCP();

        f.src =   this->dst_int;
        f.dst =   this->src_int;
        f.proto = this->type;

        f.srcport = (int) this->dstport;
        f.dstport = (int) this->srcport;

        return 1;
}

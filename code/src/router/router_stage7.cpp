#include "router.h"

/**************************************************************************
* open a raw TCP socket with the socket API
**************************************************************************/
int RouterClass::tcp_rawAlloc(){
        tcp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        if (bind(tcp_socket, (struct sockaddr*) &eth[id], addrlen)) {
                perror("raw bind");
                exit(EXIT_FAILURE);
        }

        return tcp_socket;
}

/**************************************************************************
* Recieve a encrpyted TCP pkt from proxy or other router
* is handled in RouterClass::enc_recvRlydata()
*
* Recieve a TCP pkt from raw_socket, just like ICMP
**************************************************************************/
int RouterClass::handle_TCPFromRaw(Packet* p){
        p->parseTCP();
        // checking the destination address of each packet
        if (sendtoMe(p, tcp_socket)) {
                int incId = (id + 1) * 256 + 1;     // only one circuit, seq=1
                int outId = circMap[incId];
                __u16 outport = portMap[outId];

                LOG(logfd, "incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",
                    p->src.data(), p->dst.data(), outId);

                // change the dst
                p->changeDst("0.0.0.0");
                enc_sendRlyret(p, outport, outId);
        }

        sleep(5);
        return 1;
}


/**************************************************************************
* Ugly send to tcp raw_socket function
**************************************************************************/
int RouterClass::tcp_sendtoRaw(Packet* p){
        //construct dst ip address
        struct sockaddr_in dst_addr;
        memset(&dst_addr,'\0',sizeof(struct sockaddr_in));
        dst_addr.sin_family = AF_INET;
        dst_addr.sin_port = htons(p->dstport);
        inet_pton(AF_INET, p->dst.c_str(), &dst_addr.sin_addr);

        //struct iphdr *iph = (struct iphdr *)(p->getPacket());
        //int iphdr_len = iph->ihl * 4;
        //struct tcphdr* tcp= (struct tcphdr*)(p->getPayload());
        struct iovec iov;
        memset(&iov, 0, sizeof(struct iovec));

        struct msghdr m = {
                &dst_addr, sizeof(struct sockaddr_in), &iov,
                1, 0, 0, 0
        };

        ssize_t bs;

        iov.iov_base = p->getPayload();
        iov.iov_len = p->recheckTCP();

        if (0> (bs = sendmsg(tcp_socket, &m, 0)))
        {
                perror ("ERROR: sendmsg ()");
        }
        else
        {
                printf("\nRouter%d: send tcp packet, size:%d\n", id+1, (int)bs);
        }

        memset(&m, 0, sizeof(struct msghdr));
        return bs;
}

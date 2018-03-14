#include "router.h"

/**************************************************************************
* Reply to any ping packets that are addressed to itself
* For packets that go elsewhere, it should *rewrite* them to its own IP address
* and then send them out a raw socket to the real world
**************************************************************************/
int RouterClass::readFromProxy()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        int strLen = recvfrom(sock, buffer, BUF_SIZE, 0,
                              (struct sockaddr*) proxyAddr, (socklen_t*) &nSize);
        printf("\nRouter: Read a packet from proxy, packet length:%d\n", strLen);

        //get an ICMP ECHO packet from Proxy
        //reply to any ping packets that are addressed to itself
        Packet* p = new Packet(buffer, strLen);

        if (p->parse())
        {
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n",
                    proxyAddr->sin_port, p->src.data(), p->dst.data(), p->type);

                // Reply to any ping packets that are addressed to itself (same as stage 2)
                if (this->sendtoMe(p, sock)) {
                        printf("Router%d: icmpreply to proxy!\n", id+1);
                        //send it to the proxy
                        p->icmpReply();
                        //p->printPacket();
                        p->sendUDP(proxyAddr, sock, p->getPacket(), p->getPacketLen());
                }
                else{
                        printf("Router%d: rewrite and send!\n", id+1);
                        //save key-val
                        record(p);
                        this->rewritePkt(p);
                }

        }
        else
                fprintf(stderr, "Invalid packet!\n");
};

/**************************************************************************
* If the packet comes from proxy
* check if the packet destination is tunnel
* NOTE: when setup tunnel
*   > sudo ifconfig [tun1] 10.5.51.2/24 up
* so the destination can be 10.5.51.x, where x!=2
*
* If the packet comes from raw_sock
* check if the packet destination is eth[i]
**************************************************************************/
int RouterClass::sendtoMe(Packet* p, int socket)
{
        string dst = p->dst;
        //std::cout << dst.substr(8,dst.length()-8) << "\n";

        if (socket==sock) {
                int x=0;
                try {
                        x = stoi(dst.substr(8,dst.length()-8));
                } catch (invalid_argument& e) {
                        //std::cout << "conversion failed" << std::endl;
                }
                
                if (dst.substr(0,8)=="10.5.51." &&
                    x>0 && x<256 && x!=2) {
                        return 1;
                }
        }
        else if (socket==raw_socket) {
                if (strcmp(dst.c_str(),inet_ntoa(eth[id].sin_addr))==0)
                        return 1;
        }

        return 0;
}

/**************************************************************************
* Record the dst-src pair, so that router can figure out how to rewritePkt
* the ip address it reads from raw_socket
**************************************************************************/
int RouterClass::record(Packet* p)
{
        //addressMap[p->dst]=p->src;
        addressMap.insert(make_pair(p->dst,p->src));
        return 1;
}

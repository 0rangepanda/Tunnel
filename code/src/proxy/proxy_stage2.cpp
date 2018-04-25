#include "proxy.h"

/**************************************************************************
* NOTE: The tunnel name "tun1" is hardcoded
* It should be the same as when setting up tunnel
*   > sudo ip tuntap add dev [tun1] mode tun
*   > sudo ifconfig [tun1] 10.5.51.2/24 up
*
* Tunnel ip should also be hardcoded
**************************************************************************/
int ProxyClass::tunAlloc()
{
        tun_fd = tun_alloc(strdup("tun1"), IFF_TUN | IFF_NO_PI);
        if(tun_fd < 0)
        {
                perror("Open tunnel interface");
                exit(1);
        }
        return tun_fd;
}

/**************************************************************************
* When proxy read an ICMP packet from router
* relay it to tunnel
**************************************************************************/
int ProxyClass::handle_ICMPFromRouter(Packet* p)
{
        p->printPacket();
        LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n",
            routerAddr->sin_port, p->src.data(), p->dst.data(), p->icmptype);
        //send it to the tunnel
        write(tun_fd, p->getPacket(), p->getPacketLen());
        //delete p;
        return 1;
}

/**************************************************************************
* When proxy read a packet from tunnel
* relay it to router
**************************************************************************/
int ProxyClass::handle_ICMPFromTunnel(Packet* p)
{
        //std::cout << "parse" << "\n";
        //p->printPacket();
        LOG(logfd, "ICMP from tunnel, src: %s, dst: %s, type: %d\n",
            p->src.data(), p->dst.data(), p->icmptype);

        struct circuit* circ;
        if (stage!=8)
                circ = &circ1;
        else
                circ = flowMap[p->f];

        //TODO: send it to the router
        if (stage<4)
                p->sendUDP(routerAddr, sock, p->getPacket(), p->getPacketLen());
        else if (stage==4)
                p->sendUDP(hashDstIP(p->dst), sock,
                           p->getPacket(),p->getPacketLen());
        else if (stage==5)
                tun2Circ(circ,p->getPacket(),p->getPacketLen());
        else if (stage>5)
                enc_tun2Circ(circ,p->getPacket(),p->getPacketLen());
        //delete p;
        return 1;
}

/**************************************************************************
* For debug
**************************************************************************/
int ProxyClass::showRouterIP(){
        //printf("IP address: %s\n", inet_ntoa(routerAddr->sin_addr));
}

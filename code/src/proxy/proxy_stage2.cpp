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
* When proxy read a packet from router
* relay it to tunnel
**************************************************************************/
int ProxyClass::readFromRouter()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) routerAddr, (socklen_t*) &nSize);
        printf("\nProxy: Read a packet from Router, packet length:%d\n", strLen);

        Packet *p = new Packet(buffer, strLen);
        p->parse();
        if (p->type==1)
        {
                p->printPacket();
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n",
                    routerAddr->sin_port, p->src.data(), p->dst.data(), p->icmptype);
                //send it to the tunnel
                write(tun_fd, p->getPacket(), p->getPacketLen());
        }
        else
                fprintf(stderr, "Invalid packet!\n");
        delete p;
}

/**************************************************************************
* When proxy read a packet from tunnel
* relay it to router
**************************************************************************/
int ProxyClass::readFromTunnel()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        int nread = read(tun_fd,buffer,BUF_SIZE);

        if(nread < 0)
        {
                perror("Reading from tunnel interface");
                close(tun_fd);
                exit(1);
        }
        else
        {
                printf("\nProxy: Read a packet from tunnel, packet length:%d\n", nread);
                //get an ICMP ECHO packet from tunnel interface
                Packet *p = new Packet(buffer, nread);
                p->parse();

                if (p->type==1)
                {
                        //std::cout << "parse" << "\n";
                        //p->printPacket();
                        LOG(logfd, "ICMP from tunnel, src: %s, dst: %s, type: %d\n",
                            p->src.data(), p->dst.data(), p->icmptype);

                        //TODO: send it to the router
                        if (stage<4)
                                p->sendUDP(routerAddr, sock, p->getPacket(), p->getPacketLen());
                        else if (stage==4)
                                p->sendUDP(hashDstIP(p->dst), sock,
                                           p->getPacket(),p->getPacketLen());
                        else if (stage==5)
                                tun2Circ(&circ1,p->getPacket(),p->getPacketLen());
                        else if (stage==6)
                                enc_tun2Circ(&circ1,p->getPacket(),p->getPacketLen());

                }
                else
                        fprintf(stderr, "Invalid packet!\n");
                delete p;
        }
}

/**************************************************************************
* For debug
**************************************************************************/
int ProxyClass::showRouterIP(){
        printf("IP address: %s\n", inet_ntoa(routerAddr->sin_addr));
}

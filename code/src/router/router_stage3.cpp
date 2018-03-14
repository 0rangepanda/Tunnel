#include "router.h"


/**************************************************************************
* open a raw IP socket with the socket API
* review the raw(7) man page (man 7 raw)
* NOTE: bind this socket to the Designate eth IP
**************************************************************************/
int RouterClass::rawAlloc(){
        raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (bind(raw_socket, (struct sockaddr*) &eth[id], addrlen)) {
                perror("bind");
                exit(EXIT_FAILURE);
        }

        printf("bind to:%s ...\n", inet_ntoa(eth[id].sin_addr));
        return raw_socket;
}

/**************************************************************************
* For packets that go elsewhere, it should *rewrite* them to its own IP address
* and then send them out a raw socket to the real world
* With sendmsg the kernel fills in the IP header for you
**************************************************************************/
int RouterClass::rewritePkt(Packet* p){
        struct iovec iov;
        struct msghdr msgsent;
        struct sockaddr_in dest;

        //construct destination ip address
        memset(&dest,'\0',sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(IPPROTO_ICMP);
        inet_pton(AF_INET, p->dst.c_str(), &dest.sin_addr);

        //construct payload
        iov.iov_base = p->getPayload();
        iov.iov_len  = p->getPayloadLen();

        //construct message
        msgsent.msg_iov        = &iov;
        msgsent.msg_iovlen     = 1;
        msgsent.msg_name       = (caddr_t) &dest;
        msgsent.msg_namelen    = sizeof(struct sockaddr_in);
        msgsent.msg_control    = 0;
        msgsent.msg_controllen = 0;

        //send message
        if (sendmsg(raw_socket, &msgsent, 0)==-1) {
                perror("sendmsg");
        }
        return 1;
}


/**************************************************************************
* The router should read from the raw socket for its network device,
* checking the destination address of each packet that arrives, discarding
* it if addressed elsewhere, otherwise picking it up and handling it
**************************************************************************/
int RouterClass::readFromRaw(){
        // read raw socket
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        int buflen = read(raw_socket,buffer,BUF_SIZE);

        /* Use recvfrom()

           char *buffer = (char*)malloc(65536);
           memset(buffer,0,65536);
           struct sockaddr_in saddr;
           int saddr_len = sizeof(saddr);

           int buflen=recvfrom(raw_socket,buffer,65536,0,(struct sockaddr*)&saddr,
                            (socklen_t *)&saddr_len);

           if(buflen<0)
                return -1;
         */


        printf("Router%d: Read a packet from raw_socket, packet length:%d\n", id+1, buflen);

        Packet* p = new Packet(buffer, buflen);

        if (p->parse())
        {
                LOG(logfd, "ICMP from raw sock, src: %s, dst: %s, type: %d\n",
                    p->src.data(), p->dst.data(), p->type);
                // checking the destination address of each packet
                if (this->sendtoMe(p, raw_socket))
                {
                        //cout << "map: " << addressMap.at(p->src)<< endl;
                        //change the dst
                        if (addressMap.find(p->src) != addressMap.end())
                        {
                                p->changeDst(addressMap[p->src]);
                                // TODO: src address?
                                // use a hashmap
                                // key:dst -> val:src
                                //
                                // if: to people ping one same ip using one router
                                // router cannot figure out the icmpReply belongs to whom
                                // so: proxy should avoid this

                                //send it to the proxy
                                p->sendUDP(proxyAddr, sock, p->getPacket(), p->getPacketLen());
                        }
                        else
                                //discard the packet;
                                ;

                }
                else
                {
                        //discard the packet;
                        ;
                }

        }
        else
                fprintf(stderr, "Invalid packet!\n");

}

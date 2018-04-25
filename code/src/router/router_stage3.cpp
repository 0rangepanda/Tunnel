#include "router.h"


/**************************************************************************
* open a raw ICMP socket with the socket API
* review the raw(7) man page (man 7 raw)
* NOTE: bind this socket to the Designate eth IP
**************************************************************************/
int RouterClass::rawAlloc(){
        raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (bind(raw_socket, (struct sockaddr*) &eth[id], addrlen)) {
                perror("raw bind");
                exit(EXIT_FAILURE);
        }

        //DEBUG("Router%d bind to:%s \n", id+1, inet_ntoa(eth[id].sin_addr));
        return raw_socket;
}

/**************************************************************************
* For packets that go elsewhere, it should *rewrite* them to its own IP address
* and then send them out a raw socket to the real world
* With sendmsg the kernel fills in the IP header for you
**************************************************************************/
int RouterClass::sendtoRaw(Packet* p){
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
        else
                printf("Rewrite msg to the Internet!\n");
        return 1;
}


/**************************************************************************
* The router should read from the raw socket for its network device,
* checking the destination address of each packet that arrives, discarding
* it if addressed elsewhere, otherwise picking it up and handling it
**************************************************************************/
int RouterClass::handle_ICMPFromRaw(Packet* p){

        LOG(logfd, "ICMP from raw sock, src: %s, dst: %s, type: %d\n",
            p->src.data(), p->dst.data(), p->icmptype);
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

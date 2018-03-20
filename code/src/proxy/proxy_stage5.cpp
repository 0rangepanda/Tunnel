#include "proxy.h"

/**************************************************************************
* The proxy will build the circuit, by picking the hops, then building it,
* hop-by-hop. Your proxy should log each hop of the whole circuit: hop: N, router: M.
**************************************************************************/
int ProxyClass::buildCirc(int minitor_hops)
{
        printf("\n-------------------Starting build circuit---------------------\n");
        //First, randomly generate the hop order list
        circ1.hops = (int *)malloc(sizeof(int)*minitor_hops);
        circ1.id  = 0;
        circ1.seq = 1;

        int* tmp = (int *)malloc(sizeof(int)*num_routers);
        srand(time(0));
        for(int i=0; i<num_routers; ++i) tmp[i]=i;
        for(int i=num_routers-1; i>=1; --i) swap(tmp[i], tmp[rand()%i]);

        for(int i=0; i<minitor_hops; ++i)
        {
                circ1.hops[i] = tmp[i];
                LOG(logfd, "hop: %d, router: %d\n", i+1, circ1.hops[i]+1);
        }

        //Then, build the circuit
        int count = minitor_hops;
        while (count) {
                count--;
                printf("count: %d\n", count);
                if (count==0)
                        extCirc(&circ1, -1);
                else
                        extCirc(&circ1, circ1.hops[minitor_hops-count]);
        }

        printf("Last hop: router%d!\n", circ1.hops[minitor_hops-1]+1);
        printf("------------------------Build circuit done----------------------\n");
        return 1;
}

/**************************************************************************
* Each step, it adds one hop to the circuit, then it extends the circuit
* through the (partial) circuit to the next hop.
**************************************************************************/
int ProxyClass::extCirc(struct circuit* circ, int next_hop)
{
        CtlmsgClass* ctlmsg = new CtlmsgClass(ext_ctl);
        ctlmsg->setCircId(circ->id,circ->seq);
        // NOTE: Circuit IDs should be i∗256+s, where i is the router id (1 to N).
        // 0 if it’s the proxy
        // s is a seq number starting at 1
        // TODO: will be changed in multi-circuit case

        __u16 port;
        if (next_hop>=0)
                port = unsigned(routers_port[next_hop]);//(__u16) routers_port[next_hop];
        else
                //NEXT-NAME should be 0xffff if this is the last hop of the circuit
                port = (__u16) 0xffff;
        ctlmsg->setCtlMsg(port);

        // send the ctlmsg to the first_hop
        struct sockaddr_in* first_router = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        first_router->sin_family = AF_INET;
        first_router->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        first_router->sin_port = routers_port[circ->hops[0]];

        printf("send:%s\n", packet_str(ctlmsg->packet, ctlmsg->packet_len));
        sendto(sock, ctlmsg->packet, ctlmsg->packet_len, 0, (struct sockaddr*)first_router, addrlen);
        // wait for extend-done msg


        // recv extdone msg from the first_hop
        char buffer[BUF_SIZE];
        int recvlen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)first_router, (socklen_t *)&nSize);
        CtlmsgClass* recv_ctlmsg = new CtlmsgClass(buffer, recvlen);

        if (recv_ctlmsg->getType()==ext_done) {
                LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
                    first_router->sin_port, recvlen, packet_str(buffer, recvlen));
                LOG(logfd, "incoming extend-done circuit, incoming: 0x%d from port: %d\n",
                    recv_ctlmsg->getCircId(), first_router->sin_port);
        }
        else
                exit(-1);

        return 1;
}

/**************************************************************************
* readFromRouter() for stage 5
**************************************************************************/
int ProxyClass::readFromRouter_5()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        int len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) routerAddr, (socklen_t*) &nSize);
        LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
            routerAddr->sin_port, len, packet_str(buffer, len));

        CtlmsgClass* ctlmsg = new CtlmsgClass(buffer, len);
        Packet *p = new Packet(ctlmsg->getPayload(), ctlmsg->getPayloadLen());

        if (p->parse())
        {
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n", routerAddr->sin_port, p->src.data(), p->dst.data(), p->type);
                //send it to the tunnel
                write(tun_fd, p->getPacket(), p->getPacketLen());
        }
        else
                fprintf(stderr, "Invalid packet!\n");
        delete p;
        return 1;
}

/**************************************************************************
* To send a packet over a circuit, the proxy will embed the IP packet in a
* relay data control message.
**************************************************************************/
int ProxyClass::tun2Circ(struct circuit* circ, char* packet, int len)
{
        CtlmsgClass* ctlmsg = new CtlmsgClass(rly_data);
        ctlmsg->setPayload(packet, len);
        ctlmsg->setCircId(circ->id,circ->seq);
        sendtoCirc(circ, ctlmsg->packet, ctlmsg->packet_len);
        return 1;
}

#include "router.h"

/**************************************************************************
* readFromProxy() for stage 5
* NOTE: UDP sended by other routers will also comes from sock
**************************************************************************/
int RouterClass::handle_Ctlmsg_5(Packet *p, struct sockaddr_in *proxyAddr) {
        /*
           char buffer[BUF_SIZE];
           memset(&buffer, 0, sizeof(buffer));
           int len = recvfrom(sock, buffer, BUF_SIZE, 0,
                           (struct sockaddr*) proxyAddr, (socklen_t*) &nSize);
         */
        LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
            proxyAddr->sin_port, p->getPacketLen(), packet_str(p->getPacket(), p->getPacketLen()));

        CtlmsgClass *ctlmsg = new CtlmsgClass(p->getPacket(), p->getPacketLen());

        if (ctlmsg->ifValid()) {
                recvCtlmsg(ctlmsg, proxyAddr->sin_port);
        } else
                fprintf(stderr, "Invalid packet!\n");

        return 1;
};

/**************************************************************************
* A type filter using switch()
**************************************************************************/
int RouterClass::recvCtlmsg(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        printf("Router %d recieve: 0x%x\n", id + 1, recv_ctlmsg->getType());

        switch (recv_ctlmsg->getType()) {
        case rly_data:
                recvRlydata(recv_ctlmsg, inc_port);
                break;

        case ext_ctl:
                recvExtCtl(recv_ctlmsg, inc_port);
                break;

        case ext_done:
                recvExtDone(recv_ctlmsg, inc_port);
                break;

        case rly_return:
                recvRlyret(recv_ctlmsg, inc_port);
                break;

        default:
                perror("Wrong contrl message type!");
                return 0;
                break;
        }
        return 1;
}

/**************************************************************************
* On the router side, when receiving a circuit-extend message,
*
* if it has not seen before,
* it should remember that it now knows about a new circuit.
*
* if the router already knows about that circuit ID (in other words, it
* remembers it and knows that there is an existing next hop), then that router
* will relay the message down the circuit, to the next-hop router it saved
* before. When doing so, it must map the incoming circuit ID (0xIDi) to the
* outgoing circuit ID (0xIDo), and pass on NEXT-NAME for the new next hop.
**************************************************************************/
int RouterClass::recvExtCtl(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        int incId = recv_ctlmsg->getCircId();
        int seq = recv_ctlmsg->getSeq();
        int inId = recv_ctlmsg->getId();
        __u16 nextport = recv_ctlmsg->getNextName();

        struct sockaddr_in *outaddr =
                (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        outaddr->sin_family = AF_INET;
        outaddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (circMap.find(incId) == circMap.end())
        // has not seen before
        {
                int outId = (id + 1) * 256 + seq;
                circMap.insert(make_pair(outId, incId));
                circMap.insert(make_pair(incId, outId));
                portMap.insert(make_pair(incId, inc_port));
                portMap.insert(make_pair(outId, nextport));

                LOG(logfd, "new extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",
                    incId, outId, nextport);

                // reply a extend-done msg
                CtlmsgClass *donemsg = new CtlmsgClass(ext_done);
                donemsg->setCircId(inId, seq);
                outaddr->sin_port = inc_port;
                sendto(sock, donemsg->packet, donemsg->packet_len, 0,
                       (struct sockaddr *)outaddr, addrlen);
        } else
        // knows about that circuit ID
        {
                LOG(logfd,
                    "forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",
                    incId, circMap[incId], portMap[circMap[incId]]);

                recv_ctlmsg->setCircId(id + 1, seq);
                outaddr->sin_port = portMap[circMap[incId]];
                sendto(sock, recv_ctlmsg->packet, recv_ctlmsg->packet_len, 0,
                       (struct sockaddr *)outaddr, addrlen);
        }

        return 1;
}

/**************************************************************************
* will mapping incoming circID to outgoing circID
**************************************************************************/
int RouterClass::relayMsg(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        int incId = recv_ctlmsg->getCircId();
        int outId = circMap[incId];
        __u16 outport = portMap[outId];

        // NOTE: Very Critical here!
        // Proxy should see circuit-extend-done messages have same circID
        recv_ctlmsg->setCircId(outId);

        struct sockaddr_in *outaddr =
                (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        outaddr->sin_family = AF_INET;
        outaddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        outaddr->sin_port = outport;

        sendto(sock, recv_ctlmsg->packet, recv_ctlmsg->packet_len, 0,
               (struct sockaddr *)outaddr, addrlen);

        return 1;
}

/**************************************************************************
* Routers must also forward circuit-extend-done messages,
* mapping the circuit ID and following the chain back to the proxy
**************************************************************************/
int RouterClass::recvExtDone(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        int incId = recv_ctlmsg->getCircId();
        int outId = circMap[incId];
        __u16 outport = portMap[outId];

        LOG(logfd,
            "forwarding extend-done circuit, incoming: 0x%x, outgoing: 0x%x at %d\n",
            incId, outId, outport);

        relayMsg(recv_ctlmsg, inc_port);
        return 1;
}

/**************************************************************************
* If the circuit exists and has a non-exit next hop, the router should
* change the source IP [of the encapsulated packet] to its own
*
* When the packet arrives at the end of the circuit, as indicated by a
* NEXT-NAME of 0xffff, the router should send the packet out to the Internet. To
* do so, change the source IP address [of the encapsulated packet] to that of
* the router. Then write the packet out its raw ICMP socket.
*
* Routers should never get packets with an unknown circuit ID,
* but you should log this error case
**************************************************************************/
int RouterClass::recvRlydata(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        Packet *p =
                new Packet(recv_ctlmsg->getPayload(), recv_ctlmsg->getPayloadLen());
        p->parse();

        int incId = recv_ctlmsg->getCircId();
        if (circMap.find(incId) == circMap.end()) {
                LOG(logfd, "unknown incoming circuit: 0x%x, src: %s, dst: %s\n", incId,
                    p->src.data(), p->dst.data());
                return 0;
        }

        int outId = circMap[incId];
        __u16 outport = portMap[outId];
        ipMap.insert(make_pair(incId, p->src));
        ipMap.insert(make_pair(outId, p->dst));

        printf("Router%d, Pair: 0x%x : %s\n", id, incId, p->src.data());
        // printf("Router%d, Pair: 0x%x : %s\n", id, outId, p->dst.data());

        if (outport == 0xffff) {
                /* send to raw_socket */
                LOG(logfd,
                    "outgoing packet, circuit incoming: 0x%x, incoming src: %s, outgoing "
                    "src: %s, dst: %s\n",
                    incId, p->src.data(), selfIP.data(), p->dst.data());

                p->changeSrc(selfIP);
                sendtoRaw(p);
        } else {
                /* forward */
                LOG(logfd,
                    "relay packet, circuit incoming: 0x%x, outgoing: 0x%x, incoming src: "
                    "%s, outgoing src: %s, dst: %s\n",
                    incId, outId, p->src.data(), selfIP.data(), p->dst.data());

                p->changeSrc(selfIP);
                recv_ctlmsg->setPayload(p->getPacket(), p->getPacketLen());
                relayMsg(recv_ctlmsg, inc_port);
        }

        return 1;
}

/**************************************************************************
* The public-facing and middle-hop routers will forward relay-return packets.
* map the circuit ID backwards,
* and map the destination IP to the next hop in the circuit
**************************************************************************/
int RouterClass::recvRlyret(CtlmsgClass *recv_ctlmsg, __u16 inc_port) {
        int incId = recv_ctlmsg->getCircId();
        int outId = circMap[incId];
        __u16 outport = portMap[outId];
        string outip = ipMap[outId];

        printf("Router %d Relay return: Income Id: 0x%x\n", id + 1, outId);

        Packet *p =
                new Packet(recv_ctlmsg->getPayload(), recv_ctlmsg->getPayloadLen());
        p->parse();

        LOG(logfd,
            "relay reply packet, circuit incoming: 0x%x, outgoing: 0x%x, src: %s, "
            "incoming dst: %s, outgoing dst: %s\n",
            incId, outId, p->src.data(), p->dst.data(), outip.data());

        p->changeDst(outip);
        recv_ctlmsg->setPayload(p->getPacket(), p->getPacketLen());
        relayMsg(recv_ctlmsg, inc_port);

        return 1;
}

/**************************************************************************
* For now, we have only one circuit, so the router knows that incoming
* traffic belongs to that circuit
**************************************************************************/
int RouterClass::sendRlyret(Packet *p, __u16 inc_port, int outId) {
        CtlmsgClass *ctlmsg = new CtlmsgClass(rly_return);
        ctlmsg->setCircId(outId);
        ctlmsg->setPayload(p->getPacket(), p->getPacketLen());

        // send the ctlmsg to the first_hop
        struct sockaddr_in *outrouter =
                (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        outrouter->sin_family = AF_INET;
        outrouter->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        outrouter->sin_port = inc_port;

        sendto(sock, ctlmsg->packet, ctlmsg->packet_len, 0,
               (struct sockaddr *)outrouter, addrlen);
        return 1;
}

/**************************************************************************
* Each router will listen for data from the network on its raw device, as in
* prior stages. When this public-facing router gets an external packet
* addressed to it, it needs to return that to the proxy.
*
* To return a packet to the proxy, the router will use a relay-return-data
* control message. The relay-return-data message will have type 0x54
*
* For now, we have only one circuit, so the router knows that incoming
* traffic belongs to that circuit
**************************************************************************/
int RouterClass::handle_ICMPFromRaw_5(Packet *p) {
        int incId = (id + 1) * 256 + 1; // only one circuit, seq=1
        int outId = circMap[incId];
        __u16 outport = portMap[outId];
        printf("Router %d\n", id + 1);
        LOG(logfd, "incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",
            p->src.data(), p->dst.data(), outId);
        // checking the destination address of each packet
        if (sendtoMe(p, raw_socket)) {
                // change the dst
                p->changeDst(ipMap[outId]);
                sendRlyret(p, outport, outId);
        }
        return 1;
}

#include "router.h"

/**************************************************************************
* readFromProxy() for stage 6
**************************************************************************/
int RouterClass::readFromProxy_6()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        int len = recvfrom(sock, buffer, BUF_SIZE, 0,
                           (struct sockaddr*) proxyAddr, (socklen_t*) &nSize);

        CtlmsgClass* ctlmsg = new CtlmsgClass(buffer, len);

        LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
            proxyAddr->sin_port, len, packet_str(buffer, len));

        if (ctlmsg->ifValid())
        {
                enc_recvCtlmsg(ctlmsg, proxyAddr->sin_port);
        }
        else
                fprintf(stderr, "Invalid packet!\n");
        return 1;
};


/**************************************************************************
* A type filter using switch()
**************************************************************************/
int RouterClass::enc_recvCtlmsg(CtlmsgClass *recv_ctlmsg, __u16 inc_port)
{
        printf("Router %d recv type: 0x%x, from %d!\n", id+1, recv_ctlmsg->getType(), inc_port);
        switch (recv_ctlmsg->getType())
        {
        case enc_fake_DH:
                enc_recvFake(recv_ctlmsg, inc_port);
                break;

        case enc_rly_data:
                enc_recvRlydata(recv_ctlmsg, inc_port);
                break;

        case enc_ext_ctl:
                enc_recvExtCtl(recv_ctlmsg, inc_port);
                break;

        case enc_ext_done:
                recvExtDone(recv_ctlmsg, inc_port);
                break;

        case enc_rly_return:
                enc_recvRlyret(recv_ctlmsg, inc_port);
                break;

        default:
                perror("Wrong contrl message type!");
                return 0;
                break;
        }
        return 1;
}

/**************************************************************************
* When a router gets a fake-diffie-hellman message for a new circuit,
* it should remember the incoming circuit ID and the new session key.
*
* When a router gets a fake-diffie-hellman message for an existing circuit,
* it should onion-decrypt the key with its session key for the circuit,
* map the incoming circuit ID to its outgoing counterpart, then forward the message.
**************************************************************************/
int RouterClass::enc_recvFake(CtlmsgClass *recv_ctlmsg, __u16 inc_port)
{
        int incId     = recv_ctlmsg->getCircId();
        int seq       = recv_ctlmsg->getSeq();
        int inId      = recv_ctlmsg->getId();

        if (keyMap.find(incId) == keyMap.end())
        //has not seen before
        {
                keyMap[incId] = (char*)malloc(sizeof(char)*recv_ctlmsg->getPayloadLen());
                strncpy(keyMap[incId], recv_ctlmsg->getPayload(), recv_ctlmsg->getPayloadLen());
                LOG(logfd, "fake-diffie-hellman, new circuit incoming: 0x%x, key:%s\n",
                    incId, packet_str(recv_ctlmsg->getPayload(), recv_ctlmsg->getPayloadLen()));
        }
        else
        {
                //decrypt the key with its session key
                AesClass* aes = new AesClass();
                char *tmp; int tmp_len;
                aes->set_decrypt_key(keyMap[incId]);
                aes->decrypt((unsigned char*)recv_ctlmsg->getPayload(),
                             recv_ctlmsg->getPayloadLen(),
                             (unsigned char**)&tmp, &tmp_len);

                recv_ctlmsg->setPayload(tmp, tmp_len);

                //forwarding!
                relayMsg(recv_ctlmsg, inc_port);
                LOG(logfd, "fake-diffie-hellman, forwarding,  circuit incoming: 0x%x, key:%s\n",
                    incId, packet_str(recv_ctlmsg->getPayload(), recv_ctlmsg->getPayloadLen()));
        }

        return 1;
}

/**************************************************************************
* enc_ext_ctl message consist of
* the private (encrpyted) version of the name of the next hop in the circuit.
*
* if the router already knows about that circuit ID
* decrypt and forward
**************************************************************************/
int RouterClass::enc_recvExtCtl(CtlmsgClass* recv_ctlmsg, __u16 inc_port)
{
        int incId      = recv_ctlmsg->getCircId();
        int seq        = recv_ctlmsg->getSeq();
        int inId       = recv_ctlmsg->getId();

        struct sockaddr_in* outaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        outaddr->sin_family = AF_INET;
        outaddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        //decrypt the payload
        AesClass* aes = new AesClass();
        char *tmp; int tmp_len;
        aes->set_decrypt_key(keyMap[incId]);
        aes->decrypt((unsigned char*)recv_ctlmsg->getPayload(),
                     recv_ctlmsg->getPayloadLen(),
                     (unsigned char**)&tmp, &tmp_len);

        struct ctlmsg* tmpport = (struct ctlmsg*) tmp;
        __u16 nextport = ntohs(tmpport->next_name);

        //if I know this incId?
        if (circMap.find(incId) == circMap.end()) //has not seen before
        {
                int outId = (id+1)*256 + seq;
                circMap.insert(make_pair(outId, incId));
                circMap.insert(make_pair(incId, outId));
                portMap.insert(make_pair(incId, inc_port));
                portMap.insert(make_pair(outId, nextport));

                LOG(logfd, "new extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",
                    incId, outId, nextport);

                // reply a enc-extend-done msg
                CtlmsgClass* donemsg = new CtlmsgClass(enc_ext_done);
                donemsg->setCircId(inId,seq);
                outaddr->sin_port = inc_port;
                sendto(sock, donemsg->packet, donemsg->packet_len, 0,
                       (struct sockaddr*)outaddr, addrlen);
        }
        else //knows about that circuit ID
        {
                LOG(logfd, "forwarding extend circuit: incoming: 0x%x, outgoing: 0x%x at %d\n",
                    incId, circMap[incId], portMap[circMap[incId]]);

                recv_ctlmsg->setCircId(id+1,seq);
                outaddr->sin_port = portMap[circMap[incId]];
                recv_ctlmsg->setPayload(tmp, tmp_len);

                sendto(sock, recv_ctlmsg->packet, recv_ctlmsg->packet_len, 0,
                       (struct sockaddr*)outaddr, addrlen);
        }
        return 1;
}

/**************************************************************************
* When a packet arrives on a circuit that has a non-exit next hop, the router
* should decrypt one layer of onion routing, map the circuit ID from incoming
* to outgoing, and forward the packet to its neighbor over UDP
**************************************************************************/
int RouterClass::enc_recvRlydata(CtlmsgClass* recv_ctlmsg, __u16 inc_port)
{
        int incId     = recv_ctlmsg->getCircId();
        if (circMap.find(incId) == circMap.end())
        {
                LOG(logfd, "unknown incoming circuit: 0x%x\n",incId);
                return 0;
        }

        int outId     = circMap[incId];
        __u16 outport = portMap[outId];
        printf("incID:%x, outID:%x, outport:%d\n", incId, outId, outport);

        //decrypt the payload
        AesClass* aes = new AesClass();
        char *tmp; int tmp_len;
        aes->set_decrypt_key(keyMap[incId]);
        aes->decrypt((unsigned char*)recv_ctlmsg->getPayload(),
                     recv_ctlmsg->getPayloadLen(),
                     (unsigned char**)&tmp, &tmp_len);


        if (outport == 0xffff)
        {
                /* send to raw_socket */
                printf("0xFFFF\n");
                Packet *p = new Packet(tmp, tmp_len);
                p->parse();
                LOG(logfd, "outgoing packet, circuit incoming: 0x%x, incoming src: %s, outgoing src: %s, dst: %s\n",
                    incId, p->src.data(), selfIP.data(), p->dst.data());
                p->changeSrc(selfIP);
                rewritePkt(p);
        }
        else
        {
                /* forward */
                LOG(logfd, "relay encrypted packet, circuit incoming: 0x%x, outgoing: 0x%x\n",
                    incId, outId);

                recv_ctlmsg->setPayload(tmp, tmp_len);
                relayMsg(recv_ctlmsg, inc_port);
        }

        return 1;
}


/**************************************************************************
* Each hop, they will need to map the circuit ID backwards.
* They will also do reverse-onion-encryption.
**************************************************************************/
int RouterClass::enc_recvRlyret(CtlmsgClass* recv_ctlmsg, __u16 inc_port)
{
        int incId     = recv_ctlmsg->getCircId();
        int outId     = circMap[incId];
        __u16 outport = portMap[outId];
        string outip  = ipMap[outId];


        //decrypt the payload
        AesClass* aes = new AesClass();
        char *tmp; int tmp_len;
        aes->set_encrypt_key(keyMap[outId]);
        aes->encrpyt((unsigned char*)recv_ctlmsg->getPayload(),
                     recv_ctlmsg->getPayloadLen(),
                     (unsigned char**)&tmp, &tmp_len);

        printf("Router %d encrypted:%d\n", id+1, tmp_len);

        LOG(logfd, "relay reply packet, circuit incoming: 0x%x, outgoing: 0x%x\n",
            incId, outId);

        //relay message
        recv_ctlmsg->setPayload(tmp, tmp_len);
        relayMsg(recv_ctlmsg, inc_port);
        return 1;
}


/**************************************************************************
* For now, we have only one circuit, so the router knows that incoming
* traffic belongs to that circuit
**************************************************************************/
int RouterClass::enc_sendRlyret(Packet* p, __u16 out_port, int outId)
{
        CtlmsgClass* ctlmsg = new CtlmsgClass(enc_rly_return);
        ctlmsg->setCircId(outId);

        //encrypt the payload
        AesClass* aes = new AesClass();
        char *tmp; int tmp_len;
        aes->set_encrypt_key(keyMap[outId]);
        aes->encrpyt((unsigned char*)p->getPacket(),
                     p->getPacketLen(),
                     (unsigned char**)&tmp, &tmp_len);

        ctlmsg->setPayload(tmp, tmp_len);

        // send the ctlmsg to the neighbor_hop
        struct sockaddr_in* outrouter = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        outrouter->sin_family = AF_INET;
        outrouter->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        outrouter->sin_port = out_port;

        sendto(sock, ctlmsg->packet, ctlmsg->packet_len, 0, (struct sockaddr*)outrouter, addrlen);
        return 1;
}

/**************************************************************************
* To return a packet to the proxy, the router will use a relay-return-encrypted-data control
* message. The relay-return-encrypted-data message will have type 0x64,
* with two bytes for the circuit ID and the appended packet contents.
* It should zero the destination IP address, just as the OP zeroed the source IP address.
**************************************************************************/
int RouterClass::readFromRaw_6(){
        // read raw socket
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));
        //int buflen = read(raw_socket,buffer,BUF_SIZE);

        struct sockaddr_in saddr;
        int saddr_len = sizeof(saddr);
        int buflen=recvfrom(raw_socket,buffer,65536,0,(struct sockaddr*)&saddr,
                            (socklen_t *)&saddr_len);

        Packet* p = new Packet(buffer, buflen);
        p->parse();

        if (p->type)
        {
                // checking the destination address of each packet
                if (sendtoMe(p, raw_socket))
                {
                        int incId     = (id+1)*256 + 1;//only one circuit, seq=1
                        int outId     = circMap[incId];
                        __u16 outport = portMap[outId];

                        LOG(logfd, "incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",
                            p->src.data(), p->dst.data(), outId);

                        //change the dst
                        p->changeDst("0.0.0.0");
                        enc_sendRlyret(p, outport, outId);
                }
        }
        else
                fprintf(stderr, "Invalid packet!\n");

        sleep(5);
        return 1;
}

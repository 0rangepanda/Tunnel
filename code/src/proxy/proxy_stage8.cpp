#include "proxy.h"

/**************************************************************************
* For multicircuit, proxy should not block when building circuit
**************************************************************************/
int ProxyClass::handle_Ctlmsg_8(Packet* p, struct sockaddr_in* routerAddr)
{
        CtlmsgClass* recv_ctlmsg = new CtlmsgClass(p->getPacket(), p->getPacketLen());

        int circId = recv_ctlmsg->getCircId();

        char* payload = recv_ctlmsg->getPayload();
        int len = recv_ctlmsg->getPayloadLen();

        //need circ to decrypt
        struct circuit* circ;
        if (stage!=8)
                circ = &circ1;
        else
                circ = circpool[recv_ctlmsg->getSeq()-1];

        //the decrypt is different
        if (stage==9 && recv_ctlmsg->getType()==router_worr)
        {
                printf("Proxy: recv a router_worr msg\n");
                recvRouterWorr(recv_ctlmsg, circ); //stage 9
                return 1;
        }

        //Normal Pkt
        //onion-decrypt
        /*
           AesClass* aes = new AesClass();
           for (int i = 0; i < circ->len; ++i)
           {
                char *tmp; int tmp_len;
                aes->set_decrypt_key(circ->key[i]);
                aes->decrypt((unsigned char*)payload, len,
                             (unsigned char**)&tmp, &tmp_len);

                payload = tmp;
                len = tmp_len;
           }

           //decrypted packet
           Packet *p1 = new Packet(payload, len);
           p1->parse();*/

        //type filter
        switch (recv_ctlmsg->getType())
        {
        case enc_rly_return:
                recvRlyData(recv_ctlmsg);
                break;

        case enc_ext_done:
                recvExtDone(recv_ctlmsg);
                break;

        default:
                perror("Wrong contrl message type!");
                return 0;
                break;
        }

        return 1;
};



/**************************************************************************
* The proxy should remember the flow-to-circuit mapping so subsequent packets
* reuse the same circuit. (Note that Project B required circuit
* IDs to be i ∗ 256 + s, where i is the router id (1 to N), or 0 if it’s
* the proxy, and s is a sequential number, starting at 1. You are now going
* to increment s for each new flow.)
**************************************************************************/
int ProxyClass::handleFlow(Packet* p)
{
        DEBUG("handleFlow\n");
        //DEBUG("\n------------------------ Handle Flow -------------------------\n");
        printDebugLine("Handle Flow!", 64);

        struct flow f = p->f;
        struct circuit* circ = (struct circuit*)malloc(sizeof(struct circuit));
        //check if already in mapping
        //printf("Flow: %d, %d, %d, %d, %d\n", f.src, f.dst, f.srcport, f.dstport, f.proto);

        if (flowMap.find(f) == flowMap.end())
        {
                //new flow -> build a new circuit
                DEBUG("Build a new circuit!\n");
                buildNewCirc(minitor_hops, circ);
                circpool.push_back(circ);
                //mapping
                flowMap.insert(make_pair(f, circ));

                //TODO: push in pktpool
                char* buf = (char*)malloc(p->getPacketLen()+1);
                memcpy(buf, p->getPacket(), p->getPacketLen());

                Packet *newpkt = new Packet(buf, p->getPacketLen());
                pktpool.push_back(newpkt);

                return 0;
        }
        else
        {
                //existing flow
                //mapping
                printDebugLine("Existing Flow!", 64);
                //DEBUG("\n---------------------- Existing Flow! ------------------------\n");
                circ = flowMap[f];
                if (circ->ready==circ->len)
                        return 1; //circuit built
                else
                        return 0; //circuit not done
        }
};

/**************************************************************************
*
**************************************************************************/
int ProxyClass::recvRlyData(CtlmsgClass *recv_ctlmsg)
{
        int circId = recv_ctlmsg->getCircId();
        char* payload = recv_ctlmsg->getPayload();
        int len = recv_ctlmsg->getPayloadLen();

        struct circuit* circ;
        if (stage!=8)
                circ = &circ1;
        else
                circ = circpool[recv_ctlmsg->getSeq()-1];

        //Normal Pkt
        //onion-decrypt
        AesClass* aes = new AesClass();
        for (int i = 0; i < circ->len; ++i)
        {
                char *tmp; int tmp_len;
                aes->set_decrypt_key(circ->key[i]);
                aes->decrypt((unsigned char*)payload, len, (unsigned char**)&tmp, &tmp_len);
                payload = tmp;
                len = tmp_len;
        }

        //decrypted packet
        Packet *p1 = new Packet(payload, len);
        p1->parse();

        //handle ICMP pkt
        if (p1->type==1)
        {
                DEBUG("Map to %d -> %s\n", circ->id*256+circ->seq, srcMap[circ->id*256+circ->seq].data());
                p1->changeDst(srcMap[circId]);
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n",
                    routerAddr->sin_port, p1->src.data(), p1->dst.data(), p1->icmptype);
                //send it to the tunnel
                write(tun_fd, p1->getPacket(), p1->getPacketLen());
        }
        //handle TCP pkt
        else if (p1->type==6) {
                //p1->changeDst(srcMap[circId]);
                p1->changeDst("10.0.2.15");
                p1->recheckTCP();
                p1->parseTCP();
                printTCP(p1->getPacket(), p1->getPacketLen());

                LOG(logfd, "incoming TCP packet, circuit incoming: 0x%x, src IP/port: %s:%d, dst IP/port: %s:%d, seqno: %u, ackno: %u\n",
                    circId, p1->src.data(), p1->srcport, p1->dst.data(), p1->dstport, p1->seqno, p1->ackno);
                //send it to the tunnel
                DEBUG("Write to tun, packet length: %d\n", p1->getPacketLen());
                write(tun_fd, p1->getPacket(), p1->getPacketLen());
        }
        else
        {
                fprintf(stderr, "Invalid packet!\n");
        }
        return 1;
};



/**************************************************************************
* Init an empty circuit, should not block the proxy
**************************************************************************/
int ProxyClass::buildNewCirc(int minitor_hops, struct circuit* circ)
{
        printDebugLine("Starting build circuit!", 64);

        //First, randomly generate the hop order list
        circ->hops = (int *)malloc(sizeof(int)*minitor_hops);
        circ->id  = 0;
        circ->seq = seq++;
        circ->len = minitor_hops;
        circ->ready = 0;        //stage !=8
        circ->pkt_counter = 0;  //stage 9
        DEBUG("Circuit Seq: %d\n", circ->seq);

        int* tmp = (int *)malloc(sizeof(int)*num_routers);
        srand(time(0));
        for(int i=0; i<num_routers; ++i) tmp[i]=i;
        for(int i=num_routers-1; i>=1; --i) swap(tmp[i], tmp[rand()%i]);

        //generate key for each router
        circ->key = (char**)malloc(sizeof(char*)*minitor_hops);

        /*
           for(int i=0; i<minitor_hops; ++i)
           {
                circ->hops[i] = tmp[i];
                circ->key[i] = enc_genKey(circ->hops[i]+1);
                LOG(logfd, "hop: %d, router: %d\n", i+1, circ->hops[i]+1);
           }
         */

        int i=0, j=0;
        while (i<minitor_hops) {
                if (router_status[tmp[j]]==1) //skip the down routers
                {
                        circ->hops[i] = tmp[j];
                        circ->key[i] = enc_genKey(circ->hops[i]+1);
                        LOG(logfd, "hop: %d, router: %d\n", i+1, circ->hops[i]+1);
                        i++;
                }
                j++;
                if (j==num_routers) //not enough routers
                {
                        circ->len = i;
                }
        }

        //Then, build the circuit
        //just send the first_hop
        enc_sendKey(circ, 0);
        enc_extCirc(circ, 0);
        circ->ready += 1;

        return 1;
};

/**************************************************************************
*
**************************************************************************/
int ProxyClass::recvExtDone(CtlmsgClass *recv_ctlmsg)
{
        struct circuit* circ = circpool[recv_ctlmsg->getSeq()-1];

        if (circ->ready==circ->len)
        {
                //Build circuit done
                printDebugLine("Build circuit done!", 64);
                printf("|- Circuit: Proxy --> ");
                for (int i = 0; i < minitor_hops-1; i++) {
                        printf("R%d --> ", circ->hops[i]+1);
                }
                printf("R%d!\n", circ->hops[minitor_hops-1]+1);

                //TODO: check all the queue, find which can be sent
                for (int i = 0; i < pktpool.size(); i++)
                {
                        Packet* p = pktpool[i];
                        p->parse();
                        p->pkt2flow();
                        DEBUG("Get pkt from pool!\n");
                        //printIPhdr(p->getPacket(),p->getPacketLen());

                        struct flow f = p->f;
                        DEBUG("Flow: %d, %d, %d, %d, %d\n", f.src, f.dst, f.srcport, f.dstport, f.proto);

                        if (circ == flowMap[p->f]) {
                                //send
                                printf("send!\n");
                                if (p->type==1)
                                        handle_ICMPFromTunnel(p);
                                else if (p->type==6)
                                        if (stage>=7) handle_TCPFromTunnel(p);

                                //remove
                                pktpool.erase(pktpool.begin()+i);
                        }
                }
                return 1;
        }
        else
        {
                enc_sendKey(circ, circ->ready);
                enc_extCirc(circ, circ->ready);
                circ->ready += 1;
        }
        return 1;
}














//

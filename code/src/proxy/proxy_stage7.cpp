#include "proxy.h"

/**************************************************************************
* Proxying TCP is just like proxying ICMP: you need to
* - update the IP address,
* - recompute the packet header checksum,
* - and send it through.
*
* We can get away with this simple treatment because we’re not doing full
* proxying, and we know packets come from a single computer, so there’s
* NOTE: no need to remap ports.
**************************************************************************/
int ProxyClass::handle_Ctlmsg_7(Packet* p, struct sockaddr_in* routerAddr)
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
                //circ = flowMap[f];
                circ = circpool[recv_ctlmsg->getSeq()-1];


        //Normal Pkt
        //onion-decrypt
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

        //update packet and send it to tunnel
        Packet *p1 = new Packet(payload, len);
        p1->parse();


        LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
            routerAddr->sin_port, p1->getPayloadLen(), packet_str(p1->getPayload(), p1->getPayloadLen()));

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
                //printTCP(p1->getPacket(), p1->getPacketLen());

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


        //delete p1;
        return 1;
};


/**************************************************************************
* packets should be sent using onion routing with encryption
**************************************************************************/
int ProxyClass::handle_TCPFromTunnel(Packet *p)
{
        p->parseTCP();
        //printTCP(p->getPacket(), p->getPacketLen());

        struct circuit* circ;
        if (stage!=8)
                circ = &circ1;
        else
                circ = flowMap[p->f];

        LOG(logfd, "TCP from tunnel, src IP/port: %s:%d, dst IP/port: %s:%d, seqno: %u, ackno: %u\n",
            p->src.data(), p->srcport, p->dst.data(), p->dstport, p->seqno, p->ackno);

        enc_tun2Circ(circ,p->getPacket(),p->getPacketLen());

        return 1;
};

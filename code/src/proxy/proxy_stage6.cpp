#include "proxy.h"

/**************************************************************************
* The proxy will build the circuit, by picking the hops, then building it,
* hop-by-hop. Your proxy should log each hop of the whole circuit: hop: N, router: M.
**************************************************************************/
int ProxyClass::enc_buildCirc(int minitor_hops)
{
        printf("\n-------------------Starting build circuit---------------------\n");
        //First, randomly generate the hop order list
        circ1.hops = (int *)malloc(sizeof(int)*minitor_hops);
        circ1.id  = 0;
        circ1.seq = 1;
        circ1.len = minitor_hops;

        int* tmp = (int *)malloc(sizeof(int)*num_routers);
        srand(time(0));
        for(int i=0; i<num_routers; ++i) tmp[i]=i;
        for(int i=num_routers-1; i>=1; --i) swap(tmp[i], tmp[rand()%i]);

        //generate key for each router
        circ1.key = (char**)malloc(sizeof(char*)*minitor_hops);
        for(int i=0; i<minitor_hops; ++i)
        {
                circ1.hops[i] = tmp[i];
                circ1.key[i] = enc_genKey(circ1.hops[i]+1);
                //printf("key for %d: %s\n", circ1.hops[i]+1, packet_str(circ1.key[i], 16));
                LOG(logfd, "hop: %d, router: %d\n", i+1, circ1.hops[i]+1);
        }

        //Then, build the circuit

        for(int i=0; i<minitor_hops; ++i)
        {
                enc_sendKey(&circ1, i);
                enc_extCirc(&circ1, i);
        }

        printf("------------------------Build circuit done----------------------\n");
        printf("Last hop: router%d!\n", circ1.hops[minitor_hops-1]+1);
        return 1;
}


/**************************************************************************
* the proxy should generate a random 128-bit AES key, then exclusive-OR it
* with the 16 copies of the router number of the last hop.
* So if we’re extending to router 2,
* XOR the key with 0x02020202020202020202020202020202
**************************************************************************/
char* ProxyClass::enc_genKey(int router_id)
{
        srand(time(0));
        char* key_data = (char*)malloc(sizeof(char)*16);
        for (int i = 0; i < 16; ++i)
        {
                *(key_data+i) = (char) (rand() % 128) ^ router_id;
        }
        return key_data;
}

/**************************************************************************
* The first step of adding a hop for the OP is therefore to send the
* new key to the new hop through the circuit (not directly).
**************************************************************************/
int ProxyClass::enc_sendKey(struct circuit* circ, int hop_num)
{
        //encrypt key

        char *key;
        AesClass* aes = new AesClass();

        key = circ->key[hop_num];
        int len = 16;

        for (int i = 0; i < hop_num; ++i)
        {
                char *tmp; int tmp_len;
                aes->set_encrypt_key(circ->key[hop_num-1-i]);
                aes->encrpyt((unsigned char*)key, len,
                             (unsigned char**)&tmp, &tmp_len);
                //printf("padlen: %d\n", tmp_len - len);
                key = tmp;
                len = tmp_len;
        }

        /* decrypt
           for (int i = 0; i < hop_num; ++i)
           {
                char *tmp; int tmp_len;
                aes->set_decrypt_key(circ1.key[i]);
                aes->decrypt((unsigned char*)key, len,
                             (unsigned char**)&tmp, &tmp_len);
                key = tmp;
                len = tmp_len;
                printf("decrypt key: %s\n", packet_str(key,len));
           }
         */



        //send out
        CtlmsgClass* ctlmsg = new CtlmsgClass(enc_fake_DH);
        ctlmsg->setCircId(circ->id, circ->seq);
        ctlmsg->setPayload(key, len);

        //log
        LOG(logfd, "fnew-fake-diffie-hellman, router index: %d, circuit outgoing: 0x%x, key: 0x%s\n",
            circ->hops[hop_num]+1, ctlmsg->getCircId(), packet_str(circ->key[hop_num], 16));

        // send the ctlmsg to the first_hop
        sendtoCirc(circ, ctlmsg->packet, ctlmsg->packet_len);
        printf("send key to router %d\n", circ->hops[hop_num]+1);

        return 1;

};

/**************************************************************************
* After the key is established, the OP should send a encrypted-circuit-extend
* message with type 0x62
**************************************************************************/
int ProxyClass::enc_extCirc(struct circuit* circ, int hop_num)
{
        CtlmsgClass* ctlmsg = new CtlmsgClass(enc_ext_ctl);
        ctlmsg->setCircId(circ->id, circ->seq);

        __u16 port = (hop_num!=circ->len-1) ? (__u16) routers_port[circ->hops[hop_num+1]] : (__u16) 0xffff;

        int len = 2;
        //struct ctlmsg* tmpport;
        //tmpport->next_name = htons(port);
        //char* payload = (char* )tmpport;
        char* payload = (char*)malloc(sizeof(char)*2);
        *payload     = htons(port) & 0xff;
        *(payload+1) = htons(port) >> 8;

        printf("port: 0x%x\n", htons(port));
        printf("payload: 0x%s\n", packet_str(payload, len));

        AesClass* aes = new AesClass();
        for (int i = 0; i < hop_num+1; ++i)
        {
                char *tmp;
                int tmp_len;
                aes->set_encrypt_key(circ->key[hop_num-i]);
                aes->encrpyt((unsigned char*)payload, len,
                             (unsigned char**)&tmp, &tmp_len);

                payload = tmp;
                len = tmp_len;
        }

        ctlmsg->setPayload(payload, len);

        // send the ctlmsg to the first_hop
        sendtoCirc(circ, ctlmsg->packet, ctlmsg->packet_len);
        printf("send NEXTNAME to router %d\n", circ->hops[hop_num]+1);

        // recv enc-extdone msg from the first_hop
        recvCirc(circ);

        return 1;
};

/**************************************************************************
* To send a packet over a circuit, the proxy will embed the IP packet in a
* relay data control message.
**************************************************************************/
int ProxyClass::sendtoCirc(struct circuit* circ, char* packet, int len)
{
        // send the ctlmsg to the first_hop
        struct sockaddr_in* first_router = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        first_router->sin_family = AF_INET;
        first_router->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        first_router->sin_port = routers_port[circ->hops[0]];

        sendto(sock, packet, len, 0, (struct sockaddr*)first_router, addrlen);
        return 1;
};

/**************************************************************************
* To send a packet over a circuit, the proxy will embed the IP packet in a
* relay data control message.
**************************************************************************/
int ProxyClass::recvCirc(struct circuit* circ)
{
        struct sockaddr_in* first_router = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        first_router->sin_family = AF_INET;
        first_router->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        first_router->sin_port = routers_port[circ->hops[0]];

        char buffer[BUF_SIZE];
        int recvlen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)first_router, (socklen_t *)&nSize);
        CtlmsgClass* recv_ctlmsg = new CtlmsgClass(buffer, recvlen);

        LOG(logfd, "pkt from port: %d, length: %d, contents: 0x%s\n",
            first_router->sin_port, recvlen, packet_str(buffer, recvlen));
        if (recv_ctlmsg->getType()==enc_ext_done) {
                LOG(logfd, "incoming extend-done circuit, incoming: 0x%d from port: %d\n",
                    recv_ctlmsg->getCircId(), first_router->sin_port);
        }

        return 1;
};

/**************************************************************************
* To send a packet over a encrypted circuit, the proxy will embed the IP packet
* in a relay-encrypted-data control message type 0x61.
* To hide the identity of the originator, the OP should zero the source IP.
*
* After zeroing the source IP, the OP must onion-encrypt the contents of relayed
* packet. The proxy will encrypt the relayed packet with each of the AES keys
* along the path, starting with the last hop and working backwards.
**************************************************************************/
int ProxyClass::enc_tun2Circ(struct circuit *circ, char *packet, int len)
{
        Packet* p = new Packet(packet, len);
        p->parse();

        int cricID = circ->id*256 + circ->seq;
        srcMap.insert(make_pair(cricID, p->src));
        printf("%d -> %s\n", cricID, p->src.data());
        p->changeSrc("0.0.0.0");

        //onion-encrypt
        AesClass* aes = new AesClass();
        for (int i = 0; i < circ->len; ++i)
        {
                char *tmp; int tmp_len;
                aes->set_encrypt_key(circ->key[circ->len-i-1]);
                aes->encrpyt((unsigned char*)packet, len,
                             (unsigned char**)&tmp, &tmp_len);

                packet = tmp;
                len = tmp_len;
        }


        CtlmsgClass* ctlmsg = new CtlmsgClass(enc_rly_data);
        ctlmsg->setPayload(packet, len);
        ctlmsg->setCircId(circ->id,circ->seq);
        sendtoCirc(circ, ctlmsg->packet, ctlmsg->packet_len);
        return 1;
};

/**************************************************************************
* When the packet arrives at the proxy, the proxy will decapsulate the packet,
* decrypt each layer of onion routing, fill in the destination IP with what it
* saved for the circuit, and send it back to the tunnel interface and the
* user’s application. The proxy will then write the packet out the tunnel
* interface. It should then log incoming packet,
* circuit incoming: 0xID, src: S, dst: D,
* where S should be from the public Internet, and D the IP address of the
* VM guest’s ethernet interface.
**************************************************************************/
int ProxyClass::readFromRouter_6()
{
        char buffer[BUF_SIZE];
        memset(&buffer, 0, sizeof(buffer));

        int recvlen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) routerAddr, (socklen_t*) &nSize);
        printf("\nProxy: Read a packet from Router, packet length:%d\n", recvlen);

        CtlmsgClass* recv_ctlmsg = new CtlmsgClass(buffer, recvlen);
        int incId = recv_ctlmsg->getCircId();

        char* payload = recv_ctlmsg->getPayload();
        int len = recv_ctlmsg->getPayloadLen();

        //onion-decrypt
        AesClass* aes = new AesClass();
        for (int i = 0; i < circ1.len; ++i)
        {
                char *tmp; int tmp_len;
                aes->set_decrypt_key(circ1.key[i]);
                aes->decrypt((unsigned char*)payload, len,
                             (unsigned char**)&tmp, &tmp_len);

                payload = tmp;
                len = tmp_len;
        }


        //send to tunnel
        Packet *p = new Packet(payload, len);
        p->parse();

        if (p->type)
        {
                printf("DEBUG: %d -> %s\n", circ1.id*256+circ1.seq, srcMap[circ1.id*256+circ1.seq].data());
                p->changeDst(srcMap[circ1.id*256+circ1.seq]);
                LOG(logfd, "ICMP from port: %d, src: %s, dst: %s, type: %d\n",
                    routerAddr->sin_port, p->src.data(), p->dst.data(), p->icmptype);
                //send it to the tunnel
                write(tun_fd, p->getPacket(), p->getPacketLen());
        }
        else
                fprintf(stderr, "Invalid packet!\n");
        delete p;
}

#include "proxy.h"


int ProxyClass::setStage9()
{
        circpool.push_back(&circ1);
        return 1;
}

/**************************************************************************
* The proxy should count up to N packets for a flow. Just after forwarding
* the Nth packet, it should kill the second router on that path. It will
* determine which router hosts the second hop of that flow’s circuit and
* send killrouter control message type 0x91, directly to the mortal router.
* NOTE: directly send to the router port
**************************************************************************/
int ProxyClass::killRouter(int killid)
{
        //construct msg
        CtlmsgClass* ctlmsg = new CtlmsgClass(router_kill);
        ctlmsg->setCircId(0, 1);

        //send out
        struct sockaddr_in killaddr;

        killaddr.sin_family = AF_INET;
        killaddr.sin_addr.s_addr = routerAddr->sin_addr.s_addr;
        killaddr.sin_port = routers_port[killid];
        socklen_t addrlen = sizeof(struct sockaddr_in);

        sendto(sock, ctlmsg->packet, ctlmsg->packet_len, 0, (struct sockaddr *)&killaddr, addrlen);

        printf("\nProxy: kill Router%d\n", killid+1);
        return killid;
}

//do this once
void ProxyClass::simuFailure(Packet *p)
{
        if (killed)
        {
                return;
        }

        //struct circuit* circ = (struct circuit*)malloc(sizeof(struct circuit));
        struct circuit* circ = &circ1;
        circ->pkt_counter += 1;
        printf("circ->pkt_counter: %d\n", circ->pkt_counter);
        if (circ->pkt_counter >= die_after)
        {
                killRouter(circ->hops[1]);
                killed = 1;
        }
        return;
}

/**************************************************************************
* When the proxy gets a router-worried message, it should mark the NEXT-NAME as
* down. It should then discard the circuit from its list of active circuits.
*
* As a result, the next time traffic for that circuit arrives at the proxy
* it will be forced to rebuild a new circuit.
**************************************************************************/
int ProxyClass::recvRouterWorr(CtlmsgClass *recv_ctlmsg, struct circuit *circ)
{
        printf("recvRouterWorr!\n");

        //onion-decrypt
        char* payload = recv_ctlmsg->getPayload();
        int len = recv_ctlmsg->getPayloadLen();

        AesClass* aes = new AesClass();
        int i=0;
        while (len > sizeof(struct router_worried_msg))
        {
                char *tmp; int tmp_len;
                aes->set_decrypt_key(circ->key[i++]);
                aes->decrypt((unsigned char*)payload, len,
                             (unsigned char**)&tmp, &tmp_len);

                payload = tmp;
                len = tmp_len;
        }

        struct router_worried_msg* worr = (struct router_worried_msg*)payload;
        __u16 next_name = htons(worr->next_name);
        __u16 self_name = htons(worr->self_name);
        printf("router %d worried about %d on circuit 0x%x\n", self_name, next_name, 1);
        printf("Router %d down!\n", next_nameMap[next_name]+1);
        router_status[next_nameMap[next_name]] = 0;

        circ1_down = 1;

        return 1;
};

/**************************************************************************
*
**************************************************************************/
int ProxyClass::rebuildCirc_9()
{
        if (circ1_down)
        {
                rebuildCirc(minitor_hops, &circ1);
                //circpool.clear();
                circpool.push_back(&circ1);
                circ1_down = 0;
                return 1;
        }
        return 0;
};

int ProxyClass::rebuildCirc(int minitor_hops, struct circuit* circ)
{
        printDebugLine("Rebuild circuit!", 64);

        circ->hops = (int *)malloc(sizeof(int)*minitor_hops);
        circ->id  = 0;
        circ->seq += 1; //dont change this?
        circ->len = minitor_hops;
        circ->ready = 0;        //stage 8
        circ->pkt_counter = 0;  //stage 9
        DEBUG("Circuit Seq: %d\n", circ->seq);

        //First, randomly generate the hop order list
        int* tmp = (int *)malloc(sizeof(int)*num_routers);
        srand(time(0));
        for(int i=0; i<num_routers; ++i) tmp[i]=i;
        for(int i=num_routers-1; i>=1; --i) swap(tmp[i], tmp[rand()%i]);

        //generate key for each router
        circ->key = (char**)malloc(sizeof(char*)*minitor_hops);

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

        for(int i=0; i<circ->len; ++i)
        {
                enc_sendKey(circ, i);
                enc_extCirc(circ, i);
                recvCirc(circ);
        }
        printf("------------------------Build circuit done----------------------\n");
        /*
           enc_sendKey(circ, 0);
           enc_extCirc(circ, 0);
           circ->ready += 1;

           circpool[circ->seq-1] = circ;
         */
        return 1;
}

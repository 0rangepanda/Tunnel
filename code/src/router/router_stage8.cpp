#include "router.h"

/**************************************************************************
* For multi circuit:
* egress routers (last hop) must be able to map
* incoming packets to  -> the correct return circuit.
**************************************************************************/
int RouterClass::recOutPkt(Packet *p, int circId)
{
        p->pkt2flow();
        DEBUG("Router record Flow: %d, %d, %d, %d, %d\n", p->f.src, p->f.dst, p->f.srcport, p->f.dstport, p->f.proto);
        DEBUG("Map to --> circId: 0x%x\n", circId);

        struct flow f = p->f;
        if (flowMap.find(f) == flowMap.end())
        {
                //new flow

                //mapping
                flowMap.insert(make_pair(f, circId));
        }
        else
        {
                //existing flow
                ;
        }
        return 1;
}


/**************************************************************************
* When receiving an incoming packet, calculate the flow with reversing the
* src and dst ipaddr and port,
* Query from flowMap to get the correct circId
**************************************************************************/
int RouterClass::qryIncPkt(Packet *p)
{
        int retId;
        p->rev_pkt2flow();
        DEBUG("Router query Flow: %d, %d, %d, %d, %d\n", p->f.src, p->f.dst, p->f.srcport, p->f.dstport, p->f.proto);
        struct flow f = p->f;

        if (flowMap.find(f) == flowMap.end())
        {
                //should not happen, something wrong
                printf("Wrong incoming pkt! No flow matching!\n");
                printf("Flow: %d, %d, %d, %d, %d\n", p->f.src, p->f.dst, p->f.srcport, p->f.dstport, p->f.proto);
                return -1;
        }
        else
        {
                retId = flowMap[f];
        }
        return retId;
}


/**************************************************************************
* For multi circuit:
* Not only 1 circuit, should query flowMap to get the correct outId
**************************************************************************/
int RouterClass::handle_ICMPFromRaw_8(Packet *p) {
        // checking the destination address of each packet
        if (sendtoMe(p, raw_socket)) {
                int incId = qryIncPkt(p);
                int outId = circMap[incId];
                __u16 outport = portMap[outId];

                LOG(logfd, "incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",
                    p->src.data(), p->dst.data(), outId);

                // change the dst
                p->changeDst("0.0.0.0");
                enc_sendRlyret(p, outport, outId);
        }

        return 1;
}

/**************************************************************************
*
**************************************************************************/
int RouterClass::handle_TCPFromRaw_8(Packet* p){
        p->parseTCP();
        // checking the destination address of each packet
        if (sendtoMe(p, tcp_socket)) {
                int incId = qryIncPkt(p);
                int outId = circMap[incId];
                __u16 outport = portMap[outId];

                LOG(logfd, "incoming packet, src: %s, dst: %s, outgoing circuit: 0x%x\n",
                    p->src.data(), p->dst.data(), outId);

                // change the dst
                p->changeDst("0.0.0.0");
                enc_sendRlyret(p, outport, outId);
        }

        return 1;
}

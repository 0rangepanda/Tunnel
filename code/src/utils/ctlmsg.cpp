#include "ctlmsg.h"

/**************************************************************************
* Generate a control message
**************************************************************************/
CtlmsgClass::CtlmsgClass(int t)
{
        this->type = t;
        this->packet_len = sizeof(struct ctlmsghdr) + MAXPKTLEN; // default pktlen

        switch (this->type)
        {
        //stage 5
        case rly_data:
                break;
        case ext_ctl:
                packet_len = sizeof(struct ctlmsghdr) + sizeof(struct ctlmsg);
                break;
        case ext_done:
                packet_len = sizeof(struct ctlmsghdr);
                break;
        case rly_return:
                break;

        //stage 6
        case enc_rly_data:
                break;
        case enc_ext_ctl:
                break;
        case enc_ext_done:
                packet_len = sizeof(struct ctlmsghdr);
                break;
        case enc_rly_return:
                break;
        case enc_fake_DH:
                break;

        //stage 9
        case router_kill:
                packet_len = sizeof(struct ctlmsghdr);
                break;
        case router_worr:
                packet_len = sizeof(struct ctlmsghdr) + sizeof(struct router_worried_msg);
                break;

        default:
                perror("Wrong contrl message type!");
                this->valid = 0;
                return;
        }


        this->packet = (char*)malloc(packet_len+1);
        memset(packet, 0, packet_len+1);

        //fill IP header
        struct iphdr * iph = (struct iphdr *)packet;
        iph->protocol = 253;
        // use loop address
        iph->saddr = inet_addr("127.0.0.1");
        iph->daddr = inet_addr("127.0.0.1");

        this->ctl = (struct ctlmsghdr*)packet;
        this->ctl->type = type;
        this->valid = 1;

        return;
}


/**************************************************************************
* Recieve a contrl messeage
*  - First recieve the udp pkt
*  recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&addr, &len)
**************************************************************************/
CtlmsgClass::CtlmsgClass(char* buffer, int len)
{
        this->valid = 1;
        this->packet = buffer;
        this->packet_len = len;

        this->ctl = (struct ctlmsghdr*) buffer;
        this->type = ctl->type;
        this->seq =     (ntohs(ctl->circ_id) >> 0) & 0xFF;
        this->circ_id = (ntohs(ctl->circ_id) >> 8) & 0xFF;

        this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));

        printf("Type: 0x%x, Circ Seq: %d\n", this->type, this->seq);

        switch (this->type)
        {
        //stage 5
        case ext_ctl:
                this->msg = (struct ctlmsg*) (buffer + sizeof(struct ctlmsghdr));
                this->port = ntohs(msg->next_name);
                return;

        case ext_done:
                return;

        case rly_data:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;

        case rly_return:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;

        //stage 6
        case enc_rly_data:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;
        case enc_ext_ctl:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;
        case enc_ext_done:
                return;
        case enc_rly_return:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;
        case enc_fake_DH:
                this->payload = (char*) (buffer + sizeof(struct ctlmsghdr));
                break;

        //stage 9
        case router_kill:
                break;
        case router_worr:
                this->worr = (struct router_worried_msg*) (buffer + sizeof(struct ctlmsghdr));
                this->next_name = ntohs(worr->next_name);
                this->self_name = ntohs(worr->self_name);
                break;

        default:
                perror("Wrong contrl message type!");
                valid = 0;
                return;
        }

        return;
}


int CtlmsgClass::getType()
{
        return type;
}

/**************************************************************************
* Check if the control message custructor succeeded
* if not, shouldn't do anything with it
**************************************************************************/
int CtlmsgClass::ifValid()
{
        return valid;
}

/**************************************************************************
* Circuit IDs should be i∗256+s, where i is the router id (1 to N),
* or 0 if it’s the proxy, and s is a sequential number, starting at 1,
* of the number of circuits the assigner has created.
**************************************************************************/
int CtlmsgClass::setCircId(int Icirc_id, int Iseq)
{
        seq = Iseq;
        circ_id = Icirc_id;
        ctl->circ_id = htons(Icirc_id*256 + Iseq);
        return ctl->circ_id;
}

int CtlmsgClass::setCircId(__u16 Icirc_id)
{
        ctl->circ_id = htons(Icirc_id);
        return ctl->circ_id;
}

int CtlmsgClass::getCircId()
{
        return ntohs(ctl->circ_id);
}

int CtlmsgClass::getId()
{
        return circ_id;
}

int CtlmsgClass::getSeq()
{
        return seq;
}

/**************************************************************************
* Nextname operation for type 0x52 and 0x53
**************************************************************************/
int CtlmsgClass::setCtlMsg(__u16 port)
{
        this->port = port;
        msg = (struct ctlmsg*) (packet + sizeof(struct ctlmsghdr));
        msg->next_name = htons(port);
        return 1;
}

__u16 CtlmsgClass::getNextName()
{
        return port;
}

/**************************************************************************
* Payload operation for type 0x51 and 0x54
**************************************************************************/
int CtlmsgClass::setPayload(char* input_payload, int len)
{
        if (len > MAXPKTLEN)
                return -1;

        payload = (char*) (packet + sizeof(struct ctlmsghdr));
        memcpy(payload, input_payload, len);
        packet_len = len + sizeof(struct ctlmsghdr);
        return 1;
}

char* CtlmsgClass::getPayload()
{
        switch (this->type)
        {
        //case ext_ctl:
        //        return this->msg;
        case router_worr:
                return (char *)this->worr;

        default:
                return payload;
        }
}

int CtlmsgClass::getPayloadLen()
{
        return packet_len-sizeof(struct ctlmsghdr);
}

/**************************************************************************
* For stage 9
**************************************************************************/
int CtlmsgClass::setWorr(__u16 self_name, __u16 next_name)
{
        this->self_name = self_name;
        this->next_name = next_name;
        this->worr = (struct router_worried_msg*) (packet + sizeof(struct router_worried_msg));
        this->worr->next_name = ntohs(next_name);
        this->worr->self_name = ntohs(self_name);
        return 1;
}

__u16 CtlmsgClass::getSelf_Name()
{
        return this->self_name;
}


__u16 CtlmsgClass::getNext_Name()
{
        return this->next_name;
}

int CtlmsgClass::setPayload_worr(char* input_payload, int len)
{
        if (len > MAXPKTLEN)
                return -1;

        this->worr = (struct router_worried_msg*) (packet + sizeof(struct ctlmsghdr));
        memcpy(this->worr, input_payload, len);
        packet_len = len + sizeof(struct ctlmsghdr);
        return 1;
}





//end

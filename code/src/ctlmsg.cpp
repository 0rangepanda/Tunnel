#include "ctlmsg.h"

/**************************************************************************
* Generate a control message
**************************************************************************/
CtlmsgClass::CtlmsgClass(int t)
{
        type = t;
        packet_len = sizeof(struct ctlmsghdr) + MAXPKTLEN; // default pktlen

        switch (type)
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
        default:
                perror("Wrong contrl message type!");
                valid = 0;
                return;
        }

        packet = (char*)malloc(packet_len+1);
        memset(packet, 0, packet_len+1);
        ctl = (struct ctlmsghdr*) packet;
        ctl->type = type;
        valid = 1;

        return;
}


/**************************************************************************
* Recieve a contrl messeage
*  - First recieve the udp pkt
*  recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&addr, &len)
**************************************************************************/
CtlmsgClass::CtlmsgClass(char* buffer, int len)
{
        valid = 1;
        packet = buffer;
        packet_len = len;

        ctl = (struct ctlmsghdr*) buffer;
        type = ctl->type;
        seq =     (ntohs(ctl->circ_id) >> 0) & 0xFF;
        circ_id = (ntohs(ctl->circ_id) >> 8) & 0xFF;

        switch (type)
        {
        //stage 5
        case ext_ctl:
                msg = (struct ctlmsg*) (buffer + sizeof(struct ctlmsghdr));
                port = ntohs(msg->next_name);
                return;

        case rly_data:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
                break;

        case ext_done:
                return;

        case rly_return:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
                break;

        //stage 6
        case enc_rly_data:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
                break;
        case enc_ext_ctl:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
                break;
        case enc_ext_done:
                return;
        case enc_rly_return:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
                break;
        case enc_fake_DH:
                payload = (char*) buffer + sizeof(struct ctlmsghdr);
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
int CtlmsgClass::setCtlMsg(__u16 Iport)
{
        port = Iport;
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
        for (size_t i = 0; i <len; i++) {
                *(payload++) = *(input_payload++);
        }
        packet_len = len + sizeof(struct ctlmsghdr);
        return 1;
}

char* CtlmsgClass::getPayload()
{
        return payload;
}

int CtlmsgClass::getPayloadLen()
{
        return packet_len-sizeof(struct ctlmsghdr);
}

#ifndef _CTLMSG_H_
#define _CTLMSG_H_

#ifndef MAXPKTLEN
#define MAXPKTLEN 512
#endif /* ~MAXPKTLEN */

#include <linux/ip.h>
#include <unordered_map>

#include "common.h"
#include "utils.h"

/**************************************************************************
* Mantitor control message IP header:
* With source and destination IPs of 127.0.0.1, protocol number 253,
* and all other fields zero.
*
* Mantitor control message contents:
* As specified below. One byte of type, plus two bytes of circuit ID,
* plus different contents depending on type. For Mantitor relay messages,
* included in here will be the encapsulated IP packet (with its own IP header
* and an ICMP message). For other Mantitor messages there will be other contents.
*
* circuit-extend-cntrl type: 0x52
* circuit-extend-done  type: 0x53
* relay-data type:           0x51
* relay-return-data type:    0x54
*
* Type definitions:
* typedef unsigned char __u4;
* typedef uint8_t       __u8;
* typedef uint16_t      __u16;
* typedef uint32_t      __u32;
**************************************************************************/
enum MsgType {
        rly_data   = 0x51,
        ext_ctl    = 0x52,
        ext_done   = 0x53,
        rly_return = 0x54
};

/**************************************************************************
* Circuit message with encryption
* NOTE: ->things following the e_ctlmsghdr
*
* encrypted-circuit-extend-cntrl type: 0x62 ->the private (encrpyted) version of the name of the next hop
* encrypted-circuit-extend-done  type: 0x63
* encrypted-relay-data           type: 0x61
* encrypted-relay-return-data    type: 0x64
* fake-diffie-hellman message    type: 0x65 ->16byte new session key
**************************************************************************/
enum e_MsgType {
        enc_rly_data   = 0x61,
        enc_ext_ctl    = 0x62,
        enc_ext_done   = 0x63,
        enc_rly_return = 0x64,
        enc_fake_DH    = 0x65
};

/**************************************************************************
* Msg Header: 1 byte of type + 2 byte of circuit id
**************************************************************************/
struct ctlmsghdr
{
        __u8 type;
        __u16 circ_id; //network byte order
} __attribute__ ((packed));

/**************************************************************************
* UDP port (in network byte order) of the next hop in the circuit
* NEXT-NAME should be 0xffff if this is the last hop of the circuit
**************************************************************************/
struct ctlmsg
{
        __u16 next_name;
} __attribute__ ((packed));


/*----------------------- Contrl Message Class ------------------------------*/
class CtlmsgClass
{
public:
        CtlmsgClass(int type);              // generate a ctl message
        CtlmsgClass(char* buffer, int len); // recieve  a ctl message

        int ifValid();
        int setCircId(int router_id, int seq);
        int setCircId(__u16 circ_id);
        int setCtlMsg(__u16 port);

        int getType();
        int getCircId();
        int getId();
        int getSeq();
        __u16 getNextName();

        int setPayload(char* payload, int len);
        char* getPayload();
        int getPayloadLen();

        char* packet;
        int packet_len;

protected:

private:
        int valid;

        int circ_id;
        int seq;
        int type;
        __u16 port;

        //char* packet;
        //int packet_len;

        struct ctlmsghdr *ctl;
        union {
                struct ctlmsg* msg; //0x52
                char* payload;      //0x51 and 0x54 and for encrypted message
        };

};

#endif

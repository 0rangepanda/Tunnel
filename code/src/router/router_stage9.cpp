#include "router.h"

/**************************************************************************
* On receiving that message, the router should log “router X killed”
* and then terminate (the Unix process should exit).
**************************************************************************/
int RouterClass::recvKillMsg(CtlmsgClass *recv_ctlmsg)
{
        printf("\nRouter%d: recv killmsg!\n", this->id+1);
        LOG(logfd, "router %d killed", this->id+1);

        close(sock);
        close(raw_sock);
        close(tcp_sock);
        fclose(logfd);
        exit(0);
}

/**************************************************************************
* Routers must now detect when their peers die. To detect failure, each router should keep
* a timer when it sends proxied packet down a circuit. The timer should count down 5 seconds.
* If the router gets a return packet on the circuit, it should reset the timer. If the timer expires
* with no return traffic before 5 seconds, the router should log “router SELF-NAME worried
* about NEXT-NAME on circuit circuit-ID”. (Self-name and next-name are the UDP ports
* of the current next next-hop routers.)
**************************************************************************/

//start a timer when: sends packet down a circuit
//a.k.a. recvRlyData and send
int RouterClass::startTimer()
{
        if (timer_start==0) {
                gettimeofday(&(this->timer), NULL);
                timer_start = 1;

                printf("startTimer: %ld sec\n", this->timer.tv_sec+this->timer.tv_usec/1000000);
        }
        return 1;
}

//reset timer when: gets a return packet on the circuit
//a.k.a. recvRlyret
//but do not reset when recv another enc_rly_data
int RouterClass::resetTimer()
{
        timer_start = 0;
        return 1;
}

// so we need to calculate the timeout
// TO = 5sec - time_passed_since_timer
// if TO < 0, set TO=>0
int RouterClass::setTimeout()
{
        struct timeval now;
        gettimeofday(&now, NULL);
        //printf("Reset Now: %ld sec\n", now.tv_sec+now.tv_usec/1000000);

        //long int remain = 5000000 - (now.tv_sec-timer.tv_sec) * 1000000
        //                  - (now.tv_usec-timer.tv_usec);      // in us

        long int remain = 5 - (now.tv_sec-timer.tv_sec);

        remain = remain>0 ? remain : 0;                       //if < 0, now is already timeout

        //this->timeout.tv_sec  = remain/1000000;               //sec
        //this->timeout.tv_usec = remain - remain/1000000;    //usec
        this->timeout.tv_sec = remain;
        this->timeout.tv_usec = 0;
        printf("Remain timeout: %ld sec\n", remain);
        return 1;
}

struct timeval* RouterClass::getTimeout()
{
        if (timer_start==1)
                return &(this->timeout);
        else
                return NULL;
};


/**************************************************************************
* If timeout:
* Then it should generate a router-worried, control message type 0x92, with
* the circuit ID, SELF-NAME and NEXT-NAME,
* to indicate that itself (router ID SELF-NAME) thinks router NEXT-NAME may be down.
* It should onion-route this back to the proxy up the circuit.
**************************************************************************/
int RouterClass::sendWorryMsg()
{
        printf("sendWorryMsg!\n");
        timer_start = -1;

        int outId = (id + 1) * 256 + 1; // only one circuit, seq=1
        __u16 next_name = portMap[outId];
        __u16 self_name = selfAddr->sin_port;

        //If I am the last hop, do nothing
        if (next_name == 0xffff) {
                return 1;
        }

        LOG(logfd, "router %d worried about %d on circuit 0x%x\n", self_name, next_name, 1);

        //construct payload
        CtlmsgClass *ctlmsg = new CtlmsgClass(router_worr);
        ctlmsg->setCircId(circMap[outId]);
        ctlmsg->setWorr(self_name, next_name);

        printf("ori: %s\n", packet_str(ctlmsg->getPayload(), ctlmsg->getPayloadLen()));
        // encrypt the payload
        AesClass *aes = new AesClass();
        char *tmp;
        int tmp_len;
        aes->set_encrypt_key(keyMap[circMap[outId]]);
        aes->encrpyt((unsigned char *)ctlmsg->getPayload(), ctlmsg->getPayloadLen(),
                     (unsigned char **)&tmp, &tmp_len);

        ctlmsg->setPayload(tmp, tmp_len);

        // send the ctlmsg to the neighbor_hop
        struct sockaddr_in *outrouter = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        outrouter->sin_family = AF_INET;
        outrouter->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        outrouter->sin_port = portMap[circMap[outId]];;

        sendto(sock, ctlmsg->packet, ctlmsg->packet_len, 0,
               (struct sockaddr *)outrouter, addrlen);



        return 1;
}

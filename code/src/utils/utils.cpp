#include <string>

#include "utils.h"

/**************************************************************************
* Check a filepath, if not directory, return 0
**************************************************************************/
int isDirectory(const char *path)
{
        struct stat statbuf;
        if (stat(path, &statbuf) != 0)
                return 0;
        return S_ISDIR(statbuf.st_mode);
}

/**************************************************************************
* Check a filepath
* not exist,   return -1
* directory,   return 0
* normal file, return 1
**************************************************************************/
int checkFile(const char *path)
{
        //verify if FILEPATH exist
        if(access(path, F_OK ) != 0 ) {
                fprintf(stderr, "File doesn't exist!\n");
                return -1;
        }
        //check if FILEPATH is a directory
        if(isDirectory(path)==1) {
                fprintf(stderr, "File is a directory!\n");
                return 0;
        }
        return 1;
}

/**************************************************************************
* Parse Configuration File
**************************************************************************/
int parseConf(const char *path)
{
        ifstream fin(path);
        string line;

        while(getline(fin,line))
        {
                if (line[0]!='#')
                {
                        istringstream iss(line);
                        vector<string> tokens;
                        vector<string>::iterator i;
                        copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

                        i = tokens.begin();
                        if (*i == "stage")
                                stage = std::stoi(*++i);
                        else if (*i == "num_routers")
                                num_routers = std::stoi(*++i);
                        else if (*i == "minitor_hops")
                                minitor_hops = std::stoi(*++i);
                        else if (*i == "die_after")
                                die_after = std::stoi(*++i);
                }
        }

        //check num
        if (stage<4 && num_routers!=1)
        {
                fprintf(stderr, "Invalid router number!");
                exit(-1);
        }
        else if (stage>3 && (num_routers<1 || num_routers>6))
        {
                fprintf(stderr, "Invalid router number!");
                exit(-1);
        }
        else if (stage>4 && (minitor_hops<1 || minitor_hops>num_routers))
        {
                fprintf(stderr, "Invalid router number!");
                exit(-1);
        }

        printf("\n\n");
        printDebugLine("Program Start!", 70);
        DEBUG("Program Info: \n");
        DEBUG("stage: %d\n", stage);
        DEBUG("num_routers: %d\n", num_routers);
        if (stage>4) DEBUG("minitor_hops: %d\n", minitor_hops);
        if (stage==9) DEBUG("die_after: %d\n", die_after);
        return EXIT_SUCCESS;
}

/**************************************************************************
* A wrapper of fprintf for writing line to LOG File
**************************************************************************/
void LOG(FILE *fp, const char *format,...)
{
        //pthread_mutex_lock(&mutex);
        va_list args;
        char msg[MAXSIZE];
        int msgsize;
        va_start(args,format);
        msgsize = vsnprintf(msg,sizeof(msg),format,args);
        va_end(args);
        if(msgsize < 0)
                return;
        fprintf(fp,"%s",msg);

        //pthread_mutex_unlock(&mutex);
}

/**************************************************************************
* Print debug info nicely
**************************************************************************/
void DEBUG(const char *format,...)
{
        if(!DEBUG_FLAG)
                return;
        //pthread_mutex_lock(&mutex);
        va_list args;
        char msg[MAXSIZE];
        int msgsize;
        va_start(args,format);
        msgsize = vsnprintf(msg,sizeof(msg),format,args);
        va_end(args);
        if(msgsize < 0)
                return;
        printf("|- %s",msg);

        //pthread_mutex_unlock(&mutex);
}

/**************************************************************************
* Print a line of fixed length with some context in the middle for debug
**************************************************************************/
void printDebugLine(const char *info, int len)
{
        int line = 64 - strlen(info);
        if (line<=0)
                return;

        printf("\n");
        for (int i = 0; i < line/2; ++i)
                printf("-");

        printf(" %s ", info);

        for (int i = 0; i < line/2; ++i)
                printf("-");

        if (line%2==1)
                printf("-");

        printf("\n");
}

/**************************************************************************
* tun_alloc: allocates or reconnects to a tun/tap device.
* copy from from simpletun.c
* refer to http://backreference.org/2010/03/26/tuntap-interface-tutorial/ for more info
**************************************************************************/

int tun_alloc(char *dev, int flags)
{
        struct ifreq ifr;
        int fd, err;
        char *clonedev = (char*)"/dev/net/tun";

        if( (fd = open(clonedev, O_RDWR)) < 0 )
        {
                perror("Opening /dev/net/tun");
                return fd;
        }

        memset(&ifr, 0, sizeof(ifr));

        ifr.ifr_flags = flags;

        if (*dev)
        {
                strncpy(ifr.ifr_name, dev, IFNAMSIZ);
        }

        if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 )
        {
                perror("ioctl(TUNSETIFF)");
                close(fd);
                return err;
        }

        strcpy(dev, ifr.ifr_name);
        return fd;
}

/**************************************************************************
* Bind a socket and get a dynamic UDP port (choose by OS)
**************************************************************************/
int UDP_alloc(struct sockaddr_in *addr)
{
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        socklen_t addrlen = sizeof(struct sockaddr_in);

        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr("127.0.0.1");
        addr->sin_port = htons(0);
        bind(sock, (struct sockaddr*) addr, addrlen);
        getsockname(sock, (struct sockaddr*) addr, &addrlen);
        return sock;
}

/**************************************************************************
* Convert a packet to a hex string
**************************************************************************/
char* packet_str(char* buffer, int len)
{
        //char ret[len*2+1];
        char* ret = (char*)malloc(sizeof(char)*(len*2+1));

        for (size_t i = 0; i < len; i++) {
                sprintf(&ret[i*2], "%02X", (unsigned char) *(buffer+i));
        }
        ret[len*2] = '\0';
        return ret;
}

/*******************************************************************
* This function are used to compute the ones-complement checksum
* required by IP
*******************************************************************/
unsigned short in_cksum(unsigned short *addr, int len) {
        register int sum = 0;
        u_short answer = 0;
        register u_short *w = addr;
        register int nleft = len;

        while (nleft > 1)
        {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1)
        {
                *(u_char *) (&answer) = *(u_char *) w;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
        sum += (sum >> 16);       /* add carry */
        answer = ~sum;      /* truncate to 16 bits */

        return answer;
}

/*******************************************************************
* This function are used to compute the ones-complement checksum
* required by TCP
*******************************************************************/
unsigned short in_cksum_tcp(int src, int dst, unsigned short *addr, int len)
{
        /*
           struct psd_tcp buf;
           unsigned short ans;

           buf.src.s_addr = src;
           buf.dst.s_addr = dst;
           buf.pad = 0;
           buf.proto = IPPROTO_TCP;
           buf.tcp_len = htons(len);

           memcpy(&(buf.tcp), addr, len);
         */

        unsigned short ans;
        struct psd_tcp *buf;

        if(len%2==1)
                buf = (struct psd_tcp *)malloc(12+len+1); //padding with 0
        else
                buf = (struct psd_tcp *)malloc(12+len);

        buf->src.s_addr = src;
        buf->dst.s_addr = dst;
        buf->pad = 0;
        buf->proto = IPPROTO_TCP;
        buf->tcp_len = htons(len);

        memcpy(&(buf->tcp), addr, len);

        ans = in_cksum((unsigned short *)buf, 12 + len);
        DEBUG("tcph->check: %u\n", ans);
        fflush(stdout);
        free(buf);

        return ans;
}


/**************************************************************************
* Print info of IPheader, for debug
**************************************************************************/
void printIPhdr(char* Buffer, int Size)
{
        //unsigned short iphdrlen;
        struct sockaddr_in source,dest;

        struct iphdr *iph = (struct iphdr *)Buffer;
        //iphdrlen =iph->ihl*4;
        memset(&source, 0, sizeof(source));

        source.sin_addr.s_addr = iph->saddr;
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = iph->daddr;

        printf("IP Header\n");
        printf("   |-IP Version        : %d\n",(unsigned int)iph->version);
        printf("   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
        printf("   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
        printf("   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
        printf("   |-Identification    : %d\n",ntohs(iph->id));

        printf("   |-TTL      : %d\n",(unsigned int)iph->ttl);
        printf("   |-Protocol : %d\n",(unsigned int)iph->protocol);
        printf("   |-Checksum : %d\n",ntohs(iph->check));
        printf("   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
        printf("   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

/**************************************************************************
* Print info of a TCP packet, for debug
**************************************************************************/
void printTCP(char* Buffer, int Size)
{
        unsigned short iphdrlen;
        struct iphdr *iph = (struct iphdr *)Buffer;
        iphdrlen = iph->ihl*4;
        struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen);

        printf("\n***********************TCP Packet*************************\n");
        printIPhdr(Buffer,Size);
        printf("TCP Header\n");
        printf("   |-Source Port      : %u\n",ntohs(tcph->source));
        printf("   |-Destination Port : %u\n",ntohs(tcph->dest));
        printf("   |-Sequence Number    : %u\n",ntohl(tcph->seq));
        printf("   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
        printf("   |-Header Length      : %d DWORDS or %d BYTES\n",(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
        //fprintf(logfile,"   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
        //fprintf(logfile,"   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
        printf("   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
        printf("   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
        printf("   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
        printf("   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
        printf("   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
        printf("   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
        printf("   |-Window         : %d\n",ntohs(tcph->window));
        printf("   |-Checksum       : %d\n",ntohs(tcph->check));
        printf("   |-Urgent Pointer : %d\n",tcph->urg_ptr);
        printf("\n");

        /*
           printf("HEX:\n");

           for (size_t i = 0; i < (unsigned int)tcph->doff; i++) {
                printf("%02x ", (unsigned int)(tcph+i));
           }*/

        printf("*************************** END *****************************\n");
}

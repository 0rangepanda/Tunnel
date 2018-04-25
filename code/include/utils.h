#ifndef _UTILS_H_
#define _UTILS_H_

#include "common.h"
#include "extern.h"

/*-----------------------------Data Stuctures----------------------------------*/
/**************************************************************************
* psuedo tcp header for recompute tcp checksum
**************************************************************************/
struct psd_tcp {
        struct in_addr src;
        struct in_addr dst;
        unsigned char pad;
        unsigned char proto;
        unsigned short tcp_len;
        struct tcphdr tcp;
        //unsigned short payload[];
}__attribute__ ((packed));



/*------------------------------Functions--------------------------------------*/

extern int isDirectory(const char *path);
extern int checkFile(const char *path);
extern int parseConf(const char *path);

extern void LOG(FILE *fp,const char *format,...);
extern void DEBUG(const char *format,...);
extern void printDebugLine(const char *info, int len);

extern int tun_alloc(char *dev, int flags);
extern int UDP_alloc(struct sockaddr_in *addr);
extern char* packet_str(char* buffer, int len);

extern void printIPhdr(char* Buffer, int Size);
extern void printTCP(char* Buffer, int Size);

extern unsigned short in_cksum(unsigned short *addr, int len);
extern unsigned short in_cksum_tcp(int src, int dst, unsigned short *addr, int len);

#endif

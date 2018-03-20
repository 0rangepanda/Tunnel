#ifndef _UTILS_H_
#define _UTILS_H_

#include "common.h"
#include "extern.h"

/*------------------------------Functions--------------------------------------*/

extern int isDirectory(const char *path);
extern int checkFile(const char *path);
extern int parseConf(const char *path);
extern void LOG(FILE *fp,const char *format,...);
extern int tun_alloc(char *dev, int flags);
extern int UDP_alloc(struct sockaddr_in *addr);
extern char* packet_str(char* buffer, int len);

#endif

#ifndef _COMMON_H_
#define _COMMON_H_

#define _GLIBCXX_USE_C99 1

#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <assert.h>

#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>

#include <arpa/inet.h>
#include <net/if.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/icmp.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iterator>


using namespace std;

#ifndef NULL
#define NULL 0L
#endif /* ~NULL */

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif /* ~TRUE */

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif /* ~BUF_SIZE */

#ifndef MAXSIZE
#define MAXSIZE 2048
#endif /* ~MAXSIZE */

#ifndef MAXPATHLENGTH
#define MAXPATHLENGTH 256
#endif /* ~MAXPATHLENGTH */

#ifndef MAXPKTLEN
#define MAXPKTLEN 4096
#endif /* ~MAXPKTLEN */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 1
#endif /* ~EXIT_SUCCESS */

#ifndef IPV4_OFFSET
#define IPV4_OFFSET 20
#endif /* ~IPV4_OFFSET */


typedef unsigned char __u;   //1bit
typedef uint8_t       __u8;  //1bit
typedef uint16_t      __u16; //2bit
typedef uint32_t      __u32; //4bit

#endif

#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include "common.h"
#include "utils.h"
#include "proxy.h"
#include "router.h"

/* -------------------- Global Variables -------------------- */
int stage;
int num_routers;
int minitor_hops;
int die_after;

pid_t router_pid;
short unsigned int udp_port;

FILE *logfd;

sigset_t signal_set;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t proxy_thread, monitor_thread;

struct sockaddr_in* eth;

int DEBUG_FLAG = 1;

/* -------------- Global Variables for Proxy Only --------------- */
int sock;
int tun_fd;
struct sockaddr_in addr;

int* routers_pid;
int* routers_port;

/* -------------- Global Variables for Router Only -------------- */
int raw_sock = 0;
int tcp_sock = 0;

#endif

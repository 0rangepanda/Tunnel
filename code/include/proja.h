#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include "common.h"
#include "utils.h"
#include "proxy.h"
#include "router.h"

/* -------------------- Global Variables -------------------- */
int stage;
int num_routers;
pid_t router_pid;
short unsigned int udp_port;

FILE *logfd;

sigset_t signal_set;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t proxy_thread, monitor_thread;

/* -------------- Global Variables for Proxy Only --------------- */
int sock;
int tun_fd;
struct sockaddr_in addr;

#endif
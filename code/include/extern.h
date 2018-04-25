#ifndef _EXTERN_H_
#define _EXTERN_H_

/* -------------------- Global Variables -------------------- */
extern int stage;
extern int num_routers;
extern int minitor_hops;
extern int die_after;


extern pid_t router_pid;              //pid of Proxy
extern short unsigned int udp_port;   //port of Proxy

extern FILE *logfd;

extern sigset_t signal_set;
extern pthread_mutex_t mutex;
extern pthread_t proxy_thread, monitor_thread;

extern struct sockaddr_in* eth;

extern int DEBUG_FLAG;

/* -------------- Global Variables for Proxy Only --------------- */
extern int sock;
extern int tun_fd;
extern struct sockaddr_in addr;

extern int* routers_pid;
extern int* routers_port;

/* -------------- Global Variables for Router Only -------------- */
extern int raw_sock;
extern int tcp_sock;

#endif

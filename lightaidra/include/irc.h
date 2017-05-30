#ifndef __IRC_H_
#define __IRC_H_

#define static_rcv 32

typedef struct {
    int sockfd;
    struct hostent *sockhs;
    struct sockaddr_in sockadr;

} sock_t;

typedef struct {
    char rcv_a[128];
    char rcv_b[static_rcv];
    char rcv_c[static_rcv];
    char rcv_sa[static_rcv];
    char rcv_sb[static_rcv];
    char rcv_sc[static_rcv];
    char rcv_sd[static_rcv];
    char rcv_se[static_rcv];
} requests_t;

int max_pids, pid_status;
unsigned int recv_bytes;

char *data_ptr, channel[32];
char netbuf[sizebuf];

#endif

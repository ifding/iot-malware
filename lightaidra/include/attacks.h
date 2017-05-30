#ifndef __ATTACKS_H_
#define __ATTACKS_H_

unsigned long srchost;
unsigned int dsthost;
unsigned short uport;
unsigned int useconds;

struct send_tcp {
    struct iphdr ip;
    struct tcphdr tcp;
    char buf[20];
};

struct pseudo_header {
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
    struct tcphdr tcp;
    char buf[20];
};

#endif

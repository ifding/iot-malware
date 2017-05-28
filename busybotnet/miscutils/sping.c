/* sping.c 
*/

#include <stdio.h>
#ifdef LINUX	/* dont ask me why i did this, i was baked ;p */
#include "include/sys/types.h"
#include "include/sys/socket.h"
#include "include/netdb.h"
#include "include/netinet/in.h"
#include "include/netinet/in_systm.h"
#include "include/netinet/ip.h"
#include "include/netinet/ip_icmp.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#endif

/*
 * If your kernel doesn't muck with raw packets, #define REALLY_RAW.
 * This is probably only Linux.
 */
#ifdef REALLY_RAW
#define FIX(x)  htons(x)
#else
#define FIX(x)  (x)
#endif

int sping_main(int argc, char **argv)
{
        int s;
        char buf[400];
        struct ip *ip = (struct ip *)buf;

#ifdef LINUX
struct icmphdr *icmp = (struct icmphdr *)(ip + 1);
#else
struct icmp *icmp = (struct icmp *)(ip+1);
#endif

        struct hostent *hp;
        struct sockaddr_in dst;
        int offset;
        int on = 1;

        printf("linux port -nimrood 6.20.97\n");
        bzero(buf, sizeof buf);
#ifdef LINUX
if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
#else
if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_IP)) < 0) {
#endif
                perror("socket");
                exit(1);
        }
        if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
                perror("IP_HDRINCL");
                exit(1);
        }
        if (argc != 2) {
                fprintf(stderr, "usage: %s hostname\n", argv[0]);
                exit(1);
        }
        if ((hp = gethostbyname(argv[1])) == NULL) {
                if ((ip->ip_dst.s_addr = inet_addr(argv[1])) == -1) {
                        fprintf(stderr, "%s: unknown host\n", argv[1]);
                }
        } else {
                bcopy(hp->h_addr_list[0], &ip->ip_dst.s_addr, hp->h_length);
        }
        printf("Sending to %s\n", inet_ntoa(ip->ip_dst));
        ip->ip_v = 4;
        ip->ip_hl = sizeof *ip >> 2;
        ip->ip_tos = 0;
        ip->ip_len = FIX(sizeof buf);
        ip->ip_id = htons(4321);
        ip->ip_off = FIX(0);
        ip->ip_ttl = 255;
        ip->ip_p = 1;
#ifdef LINUX
ip->ip_csum = 0;
#else
ip->ip_sum = 0;                 /* kernel fills in */
#endif
        ip->ip_src.s_addr = 0;          /* kernel fills in */

        dst.sin_addr = ip->ip_dst;
        dst.sin_family = AF_INET;
#ifdef LINUX
icmp->type = ICMP_ECHO;
icmp->code = 0;
icmp->checksum = htons(~(ICMP_ECHO << 8));
#else
icmp->icmp_type = ICMP_ECHO;
icmp->icmp_code = 0;
icmp->icmp_cksum = htons(~(ICMP_ECHO << 8));
#endif
                /* the checksum of all 0's is easy to compute */

        for (offset = 0; offset < 65536; offset += (sizeof buf - sizeof *ip)) {
                ip->ip_off = FIX(offset >> 3);
                if (offset < 65120)
                        ip->ip_off |= FIX(IP_MF);
                else
                        ip->ip_len = FIX(418);  /* make total 65538 */
                if (sendto(s, buf, sizeof buf, 0, (struct sockaddr *)&dst,
                                        sizeof dst) < 0) {
                        fprintf(stderr, "offset %d: ", offset);
                        perror("sendto");
                }
                if (offset == 0) {
#ifdef LINUX
icmp->type = 0;
icmp->code = 0;
icmp->checksum = 0;
#else
icmp->icmp_type = 0;
icmp->icmp_code = 0;
icmp->icmp_cksum = 0;
#endif
                }
        }
}

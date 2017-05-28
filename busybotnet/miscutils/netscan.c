/*
	gcc -lpthread netscan.c -o netscan
	Tcp/Udp/Tor port scanner with: synpacket, connect TCP/UDP and socks5(tor connection) 
*/ 

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <net/if.h>
#include <pthread.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_systm.h>

#define LPORT       1
#define HPORT       65535     
#define TCPSZ		sizeof(struct iphdr)+sizeof(struct tcphdr)
#define PSESZ       sizeof(struct pseudohdr)+sizeof(struct tcphdr)
#define TORPORT     9050
#define TORCTRL     9051
#define LOCALHOST   "127.0.0.1"
#define SOCKS5      "\x05\x01\x00"
#define UDP_RESEND  6
#define UDP_PACKET  4096

/* global var */
static int verbose;
static int syn;
static int conn;
static int tor;
static int normal;
static int progress;
static int rangeport;
static int singleport;
static int specificport;
static int udp;
static int webserver;
static int banserv;

unsigned int delay=50000, timeout=1, timeout_s=1, timeout_u=200;
unsigned short min, max, port;
unsigned short index_p=0, index_o=0, index_c=0, index_f=0;
unsigned short ports[HPORT], open_p[HPORT], closed_p[HPORT], filtred_p[HPORT];
char *hostname, *eth0, *ipsource;

typedef enum { false, true } bool;

/* struct tcp syn packet */
struct pseudohdr  {
	in_addr_t src;
    in_addr_t dst;
    char padd;
    char proto;
    unsigned short len;
};

/* struct progress bar */
typedef struct {
    char start;
    char end;
    char block;
    char cursor;
    unsigned int width;
    double max;
    bool percent;
    bool update;
} bar;

/* setup char for progress bar */
void setupbar(bar * set) {
    set->start   = '[';
    set->end     = ']';
    set->block   = '=';
    set->cursor  = '>';
    set->percent = true;
    set->update  = false;
    set->max     = 100;
    set->width   = 40;
}

/* Progress bar */
void progressbar(double pos, bar * set) {
    unsigned int print = (unsigned int)(set->width*pos/set->max);
    unsigned count;
    if(set->update) {
        for(count=set->width+2+(set->percent?5:0); count; count--)
            putchar('\b');
    } else set->update = true;        
    putchar(set->start);
    count = set->width;
    for(; print>1; print--, count--)
        putchar(set->block);
    putchar((set->max == pos) ? set->block : set->cursor);
    count--;
    for(; count; count--)
        putchar(' ');
    putchar(set->end);
    if(set->percent)
        printf(" %3d%%", (int)(100*pos/set->max));
    fflush(stdout);
}

void nscanhelp() {
  //printf("[*] Network Scanner v1.0 helper %s %s\n",__TIME__, __DATE__);
	printf("  -c | --connect\tTcp protocol\n");
	printf("  -s | --syn\t\tSyn packet scanner\n");
	printf("  -t | --tor\t\tTor scanner default 127.0.0.1:9050\n");
	printf("  -u | --udp\t\tUdp protocol\n");
	printf("  -b | --banner\t\tParse service banner\n");
	printf("  -p | --port\t\tPort method A, A-B, A,B,C,D\n");
	printf("  -d | --delay\t\tDelay synpack in ms [min: 50000]\n");
	printf("  -v | --verbose\tVerbose output\n");
	printf("  -h | --help\t\tPrint help menu\n\n");
	printf("  Example: scan -s google.it\n");
	printf("           scan -c google.it\n");
	printf("           scan -t google.it\n");
	printf("           scan -c -b google.it\n");
	printf("           scan -c -p1-100 google.it\n");
	printf("           scan -c -p1,2,3,4 google.it\n");
	exit(0);
}  
 
void ctrlc(int sig) {
	printf("\n\n    CTRL+C intercepted exit scanner\n");
	exit(0);
}

/* error control on port value */
int portcontrol(char *arg) {
	if(strstr(arg,"-") != NULL) {
		rangeport = 1;
		sscanf(arg, "%hu%*c%hu", &min,&max);
		if(min >= max || min > HPORT-1 || max > HPORT || max == LPORT) {
			printf("    [RANGE-ERROR] invalid port range %s\n\n", arg);
			exit(0);
		}
		return 0;
	}
	if(strstr(arg,",") != NULL) {
		specificport = 1;
		char *p;
		p = strtok(arg, ",");
		while(p != NULL) {
			ports[index_p++] = (unsigned short)atoi(p);
			p = strtok(NULL, ",");
		}
		return 0;
	} 
	singleport = 1;
	min = atoi(arg);
	return 0;
}

int service() {
	struct servent *se;
	int i=0;
	if(!udp) {
		while((se = getservent())) {
			if(strcmp(se->s_proto, "tcp") == 0)
				i++;
		} return i;
	}
	if(udp) {
		while((se = getservent())) {
			if(strcmp(se->s_proto, "tcp") == 0)
				i++;
		} return i;	
	} return -1;
}

char* resolveHost (char *host)  { 
	struct hostent *he;
	struct in_addr a;  
    if((he = gethostbyname(host))) {
        while (*he->h_addr_list) {
            bcopy(*he->h_addr_list++, (char *) &a, sizeof(a));
            return inet_ntoa(a);
        }
    }
	return 0;
}

int cmpfunc(const void *a, const void *b) {
    return (*(unsigned short*)a - *(unsigned short*)b);
}

/* remove duplicate */
unsigned short* rmdup(unsigned short *v, int size) {
    int i,index=0;
    unsigned short *new_v = (unsigned short*)malloc(size*sizeof(unsigned short));
    if (size == 1)
		new_v[0] = v[0];
    else {
		for (i=1; i<size; i++) {
			if (v[i] != v[i-1]) 
				new_v[index++] = v[i-1];
        }
    }
    return new_v;
}

/* get banner service on open port */
char *bannerservice(unsigned short bport) {
	int sock, conn, ctra, ctrb, sendbytes, rcvdbytes;
	char banner[1000], *httpdsptr, *httpdbptr, ip_addr[16];
	struct sockaddr_in ban;
	struct hostent *host;
	struct timeval tm;	
	tm.tv_sec = 1;
	tm.tv_usec = 0;
	host = gethostbyname(hostname);
	bzero(banner,1000);
	strcpy(ip_addr, (char *)inet_ntoa(*((struct in_addr *)host->h_addr)));
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return ("N.B. error");
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(struct timeval)) == -1) 
		return ("N.B. error");
	ban.sin_family=AF_INET;
	ban.sin_port=htons(bport);
	ban.sin_addr.s_addr=inet_addr(hostname);
	if((conn = connect(sock,(struct sockaddr *)&ban, sizeof(struct sockaddr))) == -1) 
		return ("N.B. error");
	if(bport == 80) 
		sendbytes = send(sock, "HEAD / HTTP/1.0\n\n", 19, 0);
	rcvdbytes = recv(sock, banner, 1000, 0);
	if(bport == 80)  {
		httpdsptr = strstr(banner,"Server");
		for(ctra=0; ctra!=strlen(httpdsptr); ctra++) {
			if(httpdsptr[ctra] == '\n') { 
				httpdsptr[ctra] = '\0'; 
				break; 
			} 
		}
		httpdbptr = (char *)malloc(ctra-8);
		for(ctrb=0; ctrb!=ctra; ctrb++) {
			httpdbptr[ctrb] = httpdsptr[ctrb+8];
			if(httpdsptr[ctrb+8] == '\0') { 
				break; 
			} 
		}
		
		printf("["); 
		if(strlen(httpdbptr) > 0) {
			httpdbptr[strcspn(httpdbptr,"\r")] = '\0';
			printf("%s", httpdbptr);
		} else printf("N.d");
		printf("]");
		fflush(stdout);
	} else  {
		printf("[");
		if(strlen(banner) > 0) {
			banner[strcspn(banner,"\r")] = '\0';
			printf("%s", banner);
		} else printf("N.d.");
		printf("]"); 
		fflush(stdout);
	}
	close(sock);
	return 0;
}

/* statistics port status */
int statistic() {
	struct servent *se;
	unsigned short *new_o,i,total;
	new_o = rmdup(open_p, index_o+1);
	for(i=0; i<index_o; i++);
	total=i;
	qsort(new_o, total, sizeof(unsigned short), cmpfunc);
	if(verbose)
		    printf("     ****** STATISTICS ******\n\n");
	for(i=0; i<total; i++) {
		if(new_o[i] == 80)
			webserver = 1;
		if(verbose && !udp)
			printf("     OPEN\t%d", new_o[i]);
		if(verbose && udp)
			printf("     OPEN|filtred\t%d", new_o[i]);
		if(!verbose)
			printf("     OPEN\t%d", new_o[i]);
		if(!udp) {
			if((se = getservbyport(htons(new_o[i]), "tcp")))
				printf("\t%s ", se->s_name);
			else printf("\tunknown ");
		}
		if(udp) {
			if((se = getservbyport(htons(new_o[i]), "udp")))
				printf("\t%s\n", se->s_name);
			else printf("\tunknown\n");
		} 
		if(banserv || (!banserv && new_o[i] == 80)) {
			putchar('\t');
			bannerservice(new_o[i]);
		}
		putchar('\n');
	}
	if(webserver && !banserv) {
		printf("\n[*] Webserver detected: ");
		bannerservice(80);
	}
	if(syn) {
		index_o = i;
		if(normal)
			index_f = 312-index_o-index_c;
		if(rangeport) {
		    index_f = (max-min+1)-index_o-index_c;
		}
	}
	if(index_o == 0)
		printf("\n[*] ALL ports are closed.");
	printf("\n[*] Statistics: open %d closed %d filtred %d, ", index_o, index_c, index_f);	
	return 0;
}

/* get last up interface (good if you use vpn) */
int interface() {
	char  buf[8192],ip[INET6_ADDRSTRLEN];
	struct ifconf   ifc; //= {0};
	struct ifreq   *ifr = NULL;
	int sck=0,nif=0,i=0;
	struct ifreq    *item;
	struct sockaddr *addr;
	sck = socket(PF_INET, SOCK_DGRAM, 0);
	if(sck < 0) {
		perror("[ERROR] socket() interface: ");
		exit(0);
	}
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0) {
		perror("[ERROR] ioctl(SIOCGIFCONF): ");
		exit(0);
	}
	ifr = ifc.ifc_req;
	nif = ifc.ifc_len/sizeof(struct ifreq); 
	for(i = 0; i < nif; i++) {
		item = &ifr[i];
		addr = &(item->ifr_addr);
	}
	eth0 = item->ifr_name;
	ipsource = (char*)inet_ntop(AF_INET,&(((struct sockaddr_in *)addr)->sin_addr),ip, INET6_ADDRSTRLEN);
	return 0;
}

unsigned short checksum (unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

/* check private ip (nmap function)*/
int checkip(char *ip) {
	unsigned int i1[4];
	sscanf(ip,"%3u.%3u.%3u.%3u", &i1[0], &i1[1], &i1[2], &i1[3]);
	if (i1[0] >= 224)
		return 1;
	if (i1[0] >= 96 && i1[0] <= 127)
		return 1;
	if (i1[0] >= 70 && i1[0] <= 79)
		return 1;
	if (i1[0] >= 83 && i1[0] <= 95)
		return 1;			
	if (i1[0] == 172 && i1[1] >= 16 && i1[1] <= 31)
		return 1;
	if (i1[0] == 192) {
		if (i1[1] == 168)
			return 1;
		else if (i1[1] == 0 && i1[2] == 2)
			return 1;
	}
	if (i1[0] == 169 && i1[1] == 254)
		return 1;
	if (i1[0] == 204 && i1[1] == 152 && (i1[2] == 64 || i1[2] == 65))
		return 1;
	if (i1[0] == 255 && i1[2] == 255 && i1[3] == 255)
		return 1;
	return 0;
}

void synpacket(int sockfd, struct sockaddr_in sinaddr) {
	struct iphdr ip;
    struct tcphdr tcp;
    struct pseudohdr pseudo;
    int attrib;
    char *buff = (char*)malloc(TCPSZ);
    char tmp[sizeof(struct pseudohdr)+sizeof(struct tcphdr)];
    memset(&ip, 0x0, sizeof(struct iphdr));
    memset(&tcp, 0x0, sizeof(struct tcphdr));
    ip.version              = 4;
    ip.ihl                  = 5;
    ip.tot_len              = TCPSZ;
    ip.id                   = htonl(12345);
    ip.ttl                  = 255;
    ip.protocol             = IPPROTO_TCP;
    ip.saddr                = inet_addr(ipsource);
    ip.daddr                = inet_addr(hostname);
    ip.check                = checksum((unsigned short*) &ip, ip.tot_len >> 1);
    tcp.source              = (rand()%64511)+1024;
    tcp.dest                = htons(port);
    tcp.seq                 = (rand()%0xFFFFFFFF);
    tcp.ack_seq             = 0;
    tcp.doff                = 5;
    tcp.syn                 = 1;
    tcp.window              = htonl(0xffff);
    tcp.check               = 0;
    sinaddr.sin_family      = AF_INET;
    sinaddr.sin_port        = htons(port);
    sinaddr.sin_addr.s_addr = inet_addr(hostname);
    attrib                  = 1;
    memset(tmp, 0x0, sizeof(struct pseudohdr)+sizeof(struct tcphdr));
    memset(buff, 0x0, TCPSZ);
    pseudo.src   = ip.saddr;
    pseudo.dst   = ip.daddr;
    pseudo.padd  = 0;
    pseudo.proto = ip.protocol;
    pseudo.len   = htons(sizeof(struct tcphdr));
    memcpy(tmp, &pseudo, sizeof(struct pseudohdr));
    memcpy(tmp+sizeof(struct pseudohdr), &tcp, sizeof(struct tcphdr));
    tcp.check = checksum ((ushort*) tmp, (PSESZ) >> 1);
    memcpy(buff, &ip, sizeof(struct iphdr));
    memcpy(buff+sizeof(struct iphdr), &tcp, sizeof(struct tcphdr));
    if (sendto (sockfd, buff, ip.tot_len, 0, (struct sockaddr *) &sinaddr, sizeof (sinaddr)) < 0)  {
		fprintf (stderr,"*** Error in sendto: %s\n",strerror(errno));
        exit(1);
    }
    usleep(delay);
}

void* ackSniffer(void *arg)  {
    int sockfd;
    size_t sin_size=sizeof(struct sockaddr);
    struct iphdr ip;
    struct tcphdr tcp;
    struct sockaddr_in sock;
    char pack[HPORT];
    
    if((sockfd=socket (PF_INET, SOCK_RAW, IPPROTO_TCP))<0) {
        fprintf (stderr,"*** Fatal - Unable to create a raw socket: %s\n",strerror(errno));
		exit(-1);
    }
    sock.sin_family=AF_INET;
    sock.sin_port=0;
    sock.sin_addr.s_addr=inet_addr(hostname);
    while(1) {
		memset (&ip,0x0,sizeof(struct iphdr));
        memset (&tcp,0x0,sizeof(struct tcphdr));
        if(recvfrom(sockfd, pack, sizeof(pack), 0, (struct sockaddr*) &sock, &sin_size)<0) {
			fprintf (stderr,"*** Fatal - Error in recvfrom(): %s\n",strerror(errno));
            exit(-2);
        }
        memcpy (&ip,pack,sizeof(struct iphdr));
        memcpy (&tcp,pack+sizeof(struct iphdr),sizeof(struct tcphdr));
        if (ip.saddr == inet_addr(hostname) && tcp.ack && !tcp.rst && ntohs(tcp.source)){//>= min && ntohs(tcp.source) <= max) {
			if(verbose) {
				printf ("    [OPEN] ACK sniffed %s:%u\t [winsize %d] [ttl %d]\n",hostname,ntohs(tcp.source),tcp.window,ip.ttl);
				fflush(stdout);
			}
            open_p[index_o++] = ntohs(tcp.source);
		} else if (ip.saddr == inet_addr(hostname) && tcp.ack && tcp.rst) {
			closed_p[index_c++] = ntohs(tcp.source);
        }
	}
    pthread_exit(0);
}

int setupsock(struct sockaddr_in sock) {
	struct timeval tm;
	int sd, attrib = 1;
	if(syn) {
		pthread_t t;
		if(pthread_create (&t,NULL,ackSniffer,NULL)) {
			fprintf (stderr," [ERROR-ACKSNIFFER] Thread process create: [%s]\n",strerror(errno));
			exit(0);
		}			
		if((sd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
			fprintf(stderr, " [SYN - SOCKET] Unable to create raw socket: [%s]", strerror(errno));
			exit(0);
		}		
		if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &attrib, sizeof(attrib)) < 0)  {
			fprintf(stderr, " [SYN - SOCKOPT] Error in setsockopt: [%s]\n",strerror(errno));
			exit(0);
		}   
	}
	if(conn || tor) {
		tm.tv_sec = timeout;
		tm.tv_usec = 0;
		sock.sin_family = AF_INET;
		if(conn) {
			sock.sin_port = htons(port);
			sock.sin_addr.s_addr = inet_addr(hostname);
		}
		if(tor) {
			sock.sin_port = htons(TORPORT);
			sock.sin_addr.s_addr = inet_addr(LOCALHOST);
		}
		if((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			fprintf(stderr, " [CONNECT - ERROR] Unable to create socket: [%s]", strerror(errno));
			exit(0);
		}
		if(conn) {
            fd_set fdset;
            fcntl(sd, F_SETFL, O_NONBLOCK);
            connect(sd, (struct sockaddr *)&sock, sizeof(sock));
            FD_ZERO(&fdset);
            FD_SET(sd, &fdset);
            if(select(sd+1, NULL, &fdset, NULL, &tm) == 1) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if(so_error == 0) {
					if(verbose)
						printf("      OPEN\t\t%d\n", port);
                    open_p[index_o++] = port;
                } else {
					if(verbose)
						printf("      CLOSED\t\t%d\n", port);
                    closed_p[index_c++] = port;
                }
            } else {
				if(verbose)
					printf("      FILTRED\t\t%d\n", port);
                filtred_p[index_f++] = port;
            }
            close(sd);
            return 0;
        }
        if(tor) {
			if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &attrib, 4) < 0)  {
				fprintf(stderr," [TOR - SETSOCKOPT] SO_REUSEADDR: [%s]\n",strerror(errno));
				exit(0);
			}
			if(setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tm, sizeof(struct timeval)) < 0)  {
				fprintf (stderr," [TOR - SETSOCKOPT] SO_RCVTIMEO: [%s]\n",strerror(errno));
				exit(0);
			}
			if(connect(sd, (struct sockaddr*)&sock, sizeof(sock)) != 0) {
				fprintf(stderr," [TOR - CONNECT] Connect 127.0.0.1:9050: [%s]\n",strerror(errno));
				exit(0);
			}	
		}		 
	}
	return sd;
}

void udpscan(unsigned short port) {
	struct sockaddr_in myudp;  
	char buff[] = "0x0x0x0x0x0x0x0x0x0";

	int udpsock, rawsock, retry, retval, iplen;
	fd_set r;
	struct timeval mytimeout;
	struct icmp *packet;
	struct ip *iphdr;
	unsigned char recvbuff[UDP_PACKET];

	if((udpsock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("  [ERROR] Udp Socket: ");
		exit(-1);
    }
	if((rawsock = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP)) < 0) {
		perror("  [ERROR] Icmp raw_sock: ");
		exit(-1);
    }
	mytimeout.tv_sec = 2;
	mytimeout.tv_usec = 0;
	myudp.sin_family = AF_INET;
	myudp.sin_port = htons(port);
	myudp.sin_addr.s_addr = inet_addr(hostname);	
	retry = 0;
	while(retry++ < UDP_RESEND) {
		if((sendto(udpsock,buff,sizeof(buff),0x0,(struct sockaddr *)&myudp,sizeof(myudp))) < 0) {
			perror("  [ERROR] Udp Sendto: ");
			exit(-1);
		}
		FD_ZERO(&r);
		FD_SET(rawsock,&r);
		retval = select((rawsock+1),&r,NULL,NULL,&mytimeout); 
		if(retval) {
			if((recvfrom(rawsock,&recvbuff,sizeof(recvbuff),0x0,NULL,NULL)) < 0) {
				perror("  [ERROR] Udp Recv: ");
				exit(-1);
			}
			iphdr = (struct ip *)recvbuff;
			iplen = iphdr->ip_hl << 2;
			packet = (struct icmp *)(recvbuff + iplen);
			if((packet->icmp_type == ICMP_UNREACH) && (packet->icmp_code == ICMP_UNREACH_PORT))
				break;
		} else continue;
	}
	if(retry >= UDP_RESEND) {
		open_p[index_o++] = port;
		if(verbose) {
			printf("      OPEN|filtred\t%d\n", port);
			fflush(stdout);
		}
	} else closed_p[index_c++] = port;
}

int torscan() {
	struct sockaddr_in torsocks;
	struct servent *service;
	unsigned short portserv;
	char *buf = calloc(1024, sizeof(char));
    short l = strlen(hostname);
    short t;
    int x,sockt;
    
    if(checkip(hostname)) {
		printf("    [TOR-SOCKS5] Reject connection to private address\n");
		exit(0);
    }
    sockt = setupsock(torsocks);
    write(sockt, SOCKS5, 3); 
    read(sockt, buf, 1024);
    if((buf[0] != 0x05) || (buf[1] == 0xFF) || (buf[1] != 0x00)) {
		printf("Socks5 error!\n");
        exit(0);
	}
    buf[0] = 0x05; 
    buf[1] = 0x01; 
    buf[2] = 0x00; 
    buf[3] = 0x03; 
    buf[4] = l; 
    for(x=0; x<l; x++)
        buf[5+x] = hostname[x]; 
    x=l+5;
    t = htons(port);
    memcpy((buf+x), &t, 2);
    write(sockt, buf, x+2);
    read(sockt, buf, 1024);
    if((buf[0] == 0x05) && (buf[1] == 0x00)) {
		portserv = htons(port);
		if(verbose) {
			printf("      OPEN\t\t%d",port);
			fflush(stdout);
		}
		if((service = getservbyport(portserv,"tcp"))) {
			printf("\t(%s)\n",service->s_name);
			fflush(stdout);		
		} else { 
			printf("\t(unknown)\n");
			fflush(stdout);			
		}
		open_p[index_o++] = port;
		close(sockt);
		return 0;
    }
    if(verbose) {
		printf("      CLOSED|FILTRED\t\t%d\n",port);
		fflush(stdout);
	}
	closed_p[index_c++] = port;
	close(sockt);
	return 0;
}


int netscan_main(int argc, char **argv) {
	struct sockaddr_in sock;
	struct servent *se;
	struct timeval start, end;
	struct winsize term;
	char *portopt, *method;
	unsigned int mtime, seconds, useconds;
	int i, c, sockfd;
    progress = 1;
    
    bar progress;
    setupbar(&progress);
    	
    printf("[*] Network Scanner v1.0 starting at %s %s [*]\n",__TIME__,__DATE__);
	srand ((unsigned) time(NULL));
	gettimeofday(&start, NULL);
	signal(SIGINT, ctrlc);

    while (1) {
		static struct option long_options[] = {
			//{"verbose",  no_argument,       &verbose,       1 },
			{"verbose",  no_argument,       0,             'v'},
            {"syn",      no_argument,       0,             's'},
            {"connect",  no_argument,       0,             'c'},
            {"tor",      no_argument,       0,             't'},
            {"udp",      no_argument,       0,             'u'},
            {"banner",   no_argument,       0,             'b'},
            {"help",     no_argument,       0,             'h'},
            {"delay",    required_argument, 0,             'd'},
            {"port",	 required_argument, 0,             'p'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "scthvubd:p:", long_options, &option_index);
        if (c == -1)
			break;
		switch (c) {
            case 0:
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				break;
			case 's':
				if(getuid() != 0) {
					printf("    [ERROR-PERMISSION] You must to be root\n");
					exit(0);
				}
				syn = 1;
				method = "synpacket";
				interface();
				sockfd = setupsock(sock);
				break;
            case 'c':
				conn = 1;
				method = "connect";
				break;
			case 't':
				tor = 1;
				method = "tor";
				break;
			case 'u':
				udp = 1;
				method = "udp";
				break;
			case 'b':
				banserv = 1;
				break;
            case 'd':
				delay = atoi(optarg);
				break;
			case 'p':
				portcontrol(optarg);
				portopt = optarg;
				break;
			case 'v':
				verbose = 1;
				//progress = 0;
				break;
			case 'h':
				nscanhelp();
			case '?':
				nscanhelp();
				break;
            default:
				nscanhelp();
		}
	}
	
    if (optind < argc) {
		while (optind < argc)
			hostname = argv[optind++];
    } else nscanhelp();
    
    if(!(hostname = resolveHost(hostname))) {
        fprintf (stderr,"\n  [RESOLUTION-ERROR] Unable to resolve: %s\n\n",hostname);
        exit(0);
    }
    
    if ((syn && tor && conn && udp) || (syn && conn) || (syn && tor) || (tor && conn)) 
		nscanhelp();
	if ((udp && syn && conn) || (udp && syn) || (udp && conn) || (udp && tor))
		nscanhelp();
	
	if (!syn && !tor && !conn && !udp) {
		if(getuid() == 0) {
			syn = 1;
			method = "synpacket";
			interface();
			sockfd = setupsock(sock);
		} else {
			conn = 1;
			method = "connect";
		}
	}
	
	if (!rangeport && !singleport && !specificport)
		normal = 1;
	
	if(!verbose) {
		ioctl(0, TIOCGWINSZ, &term);
		if(term.ws_col <  progress.width+10)
			verbose = 1;
	}
	
	if (rangeport) {
		printf("    Host: %s  Method: %s  Port: [%d-%d]\n\n", hostname, method, min, max);
		if(!verbose) {
			printf("    ");
			progress.max = max-min;
		}
		for(port=min, i=1; port<max; port++, i++) {
			if(conn)
				setupsock(sock);
			if(tor)
				torscan();
			if(syn)
				synpacket(sockfd, sock);
			if(udp)
				udpscan(port);
			if(!verbose)
				progressbar(i, &progress);
		}
	}
	
	if (specificport) {
		printf("    Host: %s  Method: %s  Port: [", hostname, method);
		for(i=0; i<index_p; i++) {
			printf("%d", ports[i]);
			if(i != index_p-1)
				printf(",");
		}
		puts("]\n");
		if(!verbose) {
			printf("    ");
			progress.max = i-1;
		}
		for(i=0; i<index_p; i++) {
			port = ports[i];
			if(conn)
				setupsock(sock);
			if(tor)
				torscan();
			if(syn)
				synpacket(sockfd, sock);
			if(udp)
				udpscan(port);
			if(!verbose)
				progressbar(i, &progress);
		}
	}
	
	if (singleport) {
		printf("    Host: %s  Method: %s  Port: [%s]\n", hostname, method, portopt);
		port = min;
		if(conn)
			setupsock(sock);
		if(tor)
			torscan();
		if(syn)
			synpacket(sockfd,sock);
		if(udp)
			udpscan(port);
	}
	
	if (normal) {
		printf("    Host: %s  Method: %s  ports: [/etc/services]\n\n", hostname, method);		
		if(!verbose) {
			printf("    ");
			progress.max = 312;
			i = 0;
		}
		while((se = getservent())) {
			if(udp) {
				if(strcmp(se->s_proto, "udp") == 0) {
					port = ntohs(se->s_port);
					udpscan(port);
					if(!verbose)
						progressbar(++i, &progress);
				}
			}
			if(syn || conn || tor) {
				if(strcmp(se->s_proto, "tcp") == 0) {
					port = ntohs(se->s_port);
					if(conn)
						setupsock(sock);
					if(tor)
						torscan();
					if(syn)
						synpacket(sockfd, sock);
					if(!verbose)
						progressbar(++i, &progress);
				}
			}
		}
	}
	printf("\n\n");
	gettimeofday(&end, NULL);
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - end.tv_usec;
	mtime = ((seconds)*1000 + useconds/1000.0) +0.5;
	statistic();
	if((seconds/60) > 0)
		printf("scanned in %d.%d.%d min\n", seconds/60,seconds%60,abs(mtime-(seconds*1000)));
	else
		printf("scanned in %d.%d sec\n", seconds%60, abs(mtime-(seconds*1000)));
	return 0;
}



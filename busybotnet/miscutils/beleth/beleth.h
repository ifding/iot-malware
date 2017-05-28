/*
 * Beleth - SSH Dictionary Attack
 * beleth.h -- Main header and function declaration
 */

#ifndef BELETH_H
#define BELETH_H

#include "lists.h"

/* Globals */
int verbose; /* Level of verboseness */

/* Functions */
int read_wordlist(char *path);

int connect_sock(void);
int listen_sock(int backlog);
void crack_thread(struct t_ctx *c_thread);
void init_pw_tasker(int unix_fd, int threads);

void print_help(char *cmd);

/* SSH session handling functions */
int session_init(char *host, int port, LIBSSH2_SESSION *session);
void session_cleanup(int sock, LIBSSH2_SESSION *session);
int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
int drop_payload(int sock, LIBSSH2_SESSION *session, char *cmdline);

/* unicode table structure */
struct printTextFormat {
	char *tlc; /* top left corner */
	char *trc; /* top right corner */
	char *blc; /* bottom left corner */
	char *brc; /* bottom right corner */
	char *hrb; /* horizontal bar */
	char *vrb; /* vertical bar */
};

/* IPC Protocol Header Information */
#define REQ_PW	   0x01 /* Request new password to try */
#define FND_PW	   0x02 /* Found password */
#define NO_PW	   0x03 /* No PWs left... cleanup */

#define VERBOSE_ATTEMPTS 1
#define VERBOSE_DEBUG 2

#endif

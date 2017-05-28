/*
 * Beleth - SSH Dictionary Attack
 * lists.h -- Linked list header
 */

#ifndef LISTS_H
#define LISTS_H

/* Globals */
#define MAX_PW_LENGTH 51

/* thread context structure */
struct t_ctx
{
	int sock;				/* SSH connection socket */
	int fd;					/* Unix IPC Socket */
	int port;
	char host[21];
	LIBSSH2_SESSION *session;
};

struct t_ctx *t_current;

/* Linked list structure to hold the word list */
struct pw_list
{
    char pw[MAX_PW_LENGTH];
    struct pw_list *next;
};

struct pw_list *pw_head;
struct pw_list *pw_tail;

/* Thread context initialization */
int init_thread_ctx(char *host, int port, struct t_ctx *ptr);

/* Password linked list functions */
int init_pw_list(char *pw);
int add_pw_list(char *pw);
void destroy_pw_list(void);

#endif

/*
 * Beleth - SSH Dictionary Attack
 * lists.c -- Linked list functions
 */

#include <libssh2.h>
#include <stdio.h>
#include <stdlib.h>

#include "beleth.h"
#include "lists.h"

/*
 * Initiate the linked list with the first element
 * Returns -1 on error. 1 on success
 */
int init_pw_list(char *pw) {
	struct pw_list *ptr = (struct pw_list*)malloc(sizeof(struct pw_list));

	if(ptr == NULL)
	{
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] Creating password linked list failed.\n");
		return -1;
	}
	strncpy(ptr->pw, pw, MAX_PW_LENGTH);
	ptr->next = NULL;

	pw_head = pw_tail = ptr;
	return 1;
}

/*
 * Add entry to the end of the linked list
 * Returns -1 on error. 1 on success
 */
int add_pw_list(char *pw) {
	if(pw_head == NULL)
		return (init_pw_list(pw));

	struct pw_list *ptr = (struct pw_list*)malloc(sizeof(struct pw_list));
	if(ptr == NULL) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] Couldn't add password to list.\n");
		return -1;
	}

	strncpy(ptr->pw,pw,MAX_PW_LENGTH);
	ptr->next = NULL;

	pw_tail->next = ptr;
	pw_tail = ptr;

	return 1;
}

/*
 * Destroy the linked list and free the memory
 */
void destroy_pw_list(void) {
	struct pw_list *ptr = pw_head;

	while (ptr != NULL) {
		ptr = pw_head->next;
		if (pw_head != NULL)
			free(pw_head);
		pw_head = ptr;
	}
}

/*
 * Initiate the thread context
 * Returns -1 on error. 1 on success
 */
int init_thread_ctx(char *host, int port, struct t_ctx *ptr) {
	if(ptr == NULL)
	{
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] Null pointer passed to init_thread_ctx.\n");
		return -1;
	}

	if ((ptr->fd = connect_sock()) == -1) {
		if (ptr != NULL)
			free(ptr);
		return -1;
	}

	ptr->port = port;
	strncpy(ptr->host,host,sizeof(ptr->host)-1);
	ptr->session = libssh2_session_init();

	t_current = ptr;
	return 1;
}

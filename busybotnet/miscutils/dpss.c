/* Chris Bechberger
 * 
 * A distributed portscanner
 * Version 0.0.1
 *
 * Copyright (C) 2000 Chris Bechberger <crispb@geocities.com>
 *
 * This code may be used under the terms of Version 2 of the GPL,
 * read the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#define MAXLINE 1024
#define MAXPORT 1024

/* Switch to trigger verbose mode */
int verbose;

/* FD for incoming connections */
int listen_fd;

/* IP address to scan */
char scan_ip[16];

/* Next port number to scan */
int scan_port = 0;
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;

/* List of all ports successfully connected to */
int found_ports[MAXPORT];
int next = 0;
pthread_mutex_t found_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Signal to tell threads to start searching */
int start_signal;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;

/* Function to check the command line arguements
 *
 * Arguements:
 *-------------------------
 * argc -> number of arguements
 * argv -> array of arguements
 *
 * Returns:
 *-------------------------
 * 1 if arguements are valid
 * -1 if invalid
 */
int dpscheck_arguements(int argc, char **argv)
{
  int temp;

  /* Check the number of command line arguements */
  if((argc > 3) || (argc < 2)) {
    printf("Correct Usage: server ip_address [options]\n");
    printf("Options:\n");
    printf("-----------------------------------\n");
    printf("-h     Print this help menu\n");
    printf("-v     Verbose output\n\n");
    printf("Exiting...\n");
    return -1;
  }

  /* Get the address to scan */
  strncpy(scan_ip, argv[1], 15);

  /* Process command line arguements */
  for(temp = 2; temp < argc; temp++) {
    
    /* Turn on verbose mode */
    if(strcmp(argv[temp], "-v") == 0) {
      verbose = 1;
      printf("Verbose mode on.\n");
    }

    /* Print the help menu */
    if(strcmp(argv[temp], "-h") == 0) {
      printf("Correct Usage: server ip_address [options]\n");
      printf("Options:\n");
      printf("-----------------------------------\n");
      printf("-h     Print this help menu\n");
      printf("-v     Verbose output\n\n");
    }
  }
  
  /* Arguements are valid */
  return(1);
} /* dpscheck_arguements */

/* Set up the connection information
 * 
 * Arguements:
 *---------------------
 * None
 * 
 * Returns:
 *---------------------
 * file descriptor if successful
 * -1 if unsuccessful
 */
int dpss_init_connection()
{
  int listen_fd;
  int addr_length;
  struct sockaddr_in server_addr;

  /* Create Server socket */
  if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Cannot create a socket for the server, Exiting...\n");
    return -1;
  }

  /* Set up socket information */
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(0);

  /* Assign name to socket */
  if(bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
    printf("Error binding socket, Exiting...\n");
    return -1;
  }
  
  /* Print out socket number */
  addr_length = sizeof(server_addr);
  if(getsockname(listen_fd, (struct sockaddr_in *) &server_addr, &addr_length) < 0) {
    perror("Error getting Socket, exiting...");
    exit(1);
  }
  printf("Port Number is %d\n", ntohs(server_addr.sin_port));
  
  /* Return file descriptor for the socket */
  return(listen_fd);
} /* dpss_init_connection */

/* Function to control the server
 *
 * Arguements:
 *-------------------
 * None
 * 
 * Return:
 *-------------------
 * None
 *
 */
static void *control_console()
{
  char *temp;
  int c;

  printf("Control Console, Enter help for list of commands.\n");
  printf("------------------------------------------------------\n");
  while(1) {
    printf(">");
    scanf("%s", temp);
    if(strcmp(temp, "help") == 0) {
      printf("Available Commands:\n");
      printf("------------------------------------\n");
      printf("help     This listing\n");
      printf("start    Start the portscan\n");
      printf("results  Print a list of statistics\n");
      printf("quit     Exit this program\n");
    }
    if(strcmp(temp, "start") == 0) {
      pthread_cond_broadcast(&start_cond);
    }
    if(strcmp(temp, "results") == 0) {
      printf("Results of scanning %s:\n", scan_ip);
      printf("------------------------------------\n");
      for(c = 0; c < next; c++) {
	printf("Port %d\n", found_ports[c]);
      }
    }
    if(strcmp(temp, "quit") == 0) {
      printf("Server shutting down\n");
      close(listen_fd);
      exit(1);
    }
  }
  
  if(pthread_cond_broadcast(&start_cond) != 0) {
    printf("Cannot start scanner, Exiting...\n");
    exit(1);
  }

  return NULL;
} /* control_console */

/* Function to send next port to be scanned 
 *
 * Arguements:
 *------------------
 * connection_fd -> file descriptor to be written to
 *
 * Returns:
 *------------------
 * 0 if successful
 * -11 if unsuccessful
 *
 * Codes:
 *------------------
 * 101 -> Scan IP and Port specified
 */
int send_next(int connection_fd, int port)
{
  char temp[MAXLINE];
  
  /* Create packet to be sent */
  bzero(temp, MAXLINE);
  sprintf(temp, "101 %s %d 1", scan_ip, port);
  if(verbose) {
    printf("Sending %s\n", temp);
  }
  
  /* Send packet to client */
  if(write(connection_fd, temp, strlen(temp)) == -1) {
    printf("Error writing to thread %d, Exiting...\n", pthread_self());
    return -1;
  }

  /* Get response from client */
  if(read(connection_fd, temp, MAXLINE) == -1) {
    printf("Error reading from thread %d, Exiting...\n", pthread_self());
    return -1;
  }

  if(verbose) {
    printf("Got response %s\n", temp);
  }

  if(strncmp(temp, "201", 3) == 0) {
    if(verbose) {
      printf("Found something on port %d\n", port);
    }

    /* Add port number to list of ports found */
    if(pthread_mutex_lock(&found_mutex) != 0) {
      printf("Cannot get mutex, Exiting...\n");
      exit(1);
    }
    
    found_ports[next] = port;
    next++;
    
    if(pthread_mutex_unlock(&found_mutex) != 0) {
      printf("Cannot release mutex\n");
      exit(1);
    }
    
  }

  if(verbose) {
    printf("Thread %d is scanning %s port %d\n", pthread_self(), scan_ip, port);
  }
  
  return 0;
} /* send_next */

/* Function to close a connection to a client 
 *
 * Arguements:
 *--------------------
 * connection_fd -> file descriptor to client
 *
 * Returns:
 *--------------------
 * 0 if successful
 * -1 if unsuccessful
 */
int send_final(int connection_fd)
{
  if(verbose) {
    printf("Closing connection to thread %d, Exiting...\n", pthread_self());
  }
  
  /* Close file descriptor */
  if(close(connection_fd) == -1) {
    printf("Error closing file descriptor, Exiting...\n");
    return -1;
  }
  
  return 0;
} /* send_final */

/* Function to handle a client connection
 *
 * Arguements:
 *-------------------
 * connection_fd -> file descriptor for connection being handled.
 *
 * Return Values:
 *-------------------
 * 0 if successful
 * -1 if unsuccessful
 */
static void *handle_connection(void *arg)
{
  int connection_fd;
  int temp[2];
  int port = 0;
  pthread_t tid;

   /* Announce creation of new thread */ 
  if(verbose) {
    printf("New thread %d created.\n", pthread_self());
  }

   /* Get file descriptor for connection */ 
  connection_fd = (int) arg;
  
  /* Get next entry to scan */ 
  
  /* Get mutex */
  if(pthread_mutex_lock(&cond_mutex) != 0) {
    printf("Error getting conditiong mutex, Exiting...\n");
    return NULL;
  }
  
  /* Wait for signal to start scanning */ 
  if(pthread_cond_wait(&start_cond, &cond_mutex) != 0) {
    printf("Cannot set thread to sleep, Exiting thread...\n");
    return NULL;
  }

  printf("Thread %d starting portscanning.\n", pthread_self());
  
  /* Release Mutex */
  if(pthread_mutex_unlock(&cond_mutex) != 0) {
    printf("Error releasing mutex, Exiting...\n");
    return NULL;
  }
  
  while(port < MAXPORT) {
    
    /* Get number of next port to scan */
    if(pthread_mutex_lock(&port_mutex) != 0) {
      printf("Error getting mutex, Exiting...\n");
      return NULL;
    }
    
    port = scan_port;
    scan_port++;
    
    if(pthread_mutex_unlock(&port_mutex) != 0) {
      printf("Error releasing port, Exiting...\n");
      return NULL;
    }
    
    /* Send next port to be scanned */
    if(send_next(connection_fd, port) == -1) {
      printf("Error scanning port, Exiting...\n");
      continue;
    }
  }

  /* Close connections to clients */
  if(send_final(connection_fd) == -1) {
    printf("Error closing connection, Exiting...\n");
    exit(1);
  }

  printf("Thread %d is done port scanning.\n", pthread_self());

  /* End thread successfully */
  return NULL;

} /* handle_connection */

/* Function to handle client connections
 *
 * Arguements:
 *-------------------
 * None
 *
 * Return:
 *-------------------
 * None
 */
static void *connection_handler()
{
  int connection_fd;
  socklen_t client_length;
  struct sockaddr_in client_addr;
  pthread_t tid;

  /* Set up the connection */
  if((listen_fd = dpss_init_connection()) == -1) {
    printf("Error setting up connection, Exiting...\n");
    exit(0);
  }
  
  if(listen(listen_fd, 100) == -1) {
    printf("Error setting socket to listen, Exiting...\n");
    exit(0);
  }
  
  /* Watch for a connection */
  while(1) {
    client_length = sizeof(client_addr);
    
    /* Accept in incoming request */
    if((connection_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_length)) < 0) {
      printf("Error accepting connection, Exiting...\n");
      exit(0);
    }
    
    /* Create a new thread to deal with each client */
    if(pthread_create(&tid, NULL, &handle_connection, (void *) connection_fd) != 0) {
      printf("Error creating thread to handle connection, Exiting...\n");
      exit(0);
    }
  }

  printf("Port scanning finished\n");

  return NULL;
} /* connection_handler */

/* Main Function
 *
 * Arguements:
 *-------------------
 * argc -> number of arguements
 * argv -> array of arguements
 *
 * Returns:
 *-------------------
 * Nothing
 */
int dpss_main(int argc, char **argv)
{
  pthread_t control_tid, connection_tid;

  /* Check and process command-line arguements */
  if(dpscheck_arguements(argc, argv) == -1) {
    exit(0);
  }

  /* Create a thread to control server */
  if(pthread_create(&control_tid, NULL, &control_console, (void *) NULL) != 0) {
    printf("Cannot create thread to control server, Exiting...\n");
    exit(0);
  }

  /* Create a thread to deal with the connections */
  if(pthread_create(&connection_tid, NULL, &connection_handler, (void *) NULL) != 0) {
    printf("Cannot create thread to handle client connections, Exiting...\n");
    exit(0);
  }

  /* Join control thread to process */
  if(pthread_join(control_tid, NULL) != 0) {
    printf("Error joining control thread, Exiting...\n");
    exit(0);
  }

  /* Join connection handler thread to process */
  if(pthread_join(connection_tid, NULL) != 0) {
    printf("Error joining connection handler thread, Exiting...\n");
    exit(0);
  }

  /* End program */
  exit(1);
} /* main */

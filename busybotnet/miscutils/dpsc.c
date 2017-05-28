/* Chris Bechberger
 *
 * Client for distributed portscanner
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

#define MAXLINE 1024

/* Flag for verbose mode */
int verbose;
/* IP address of server */
char ipaddress[16];
int port;

/* Function to check the command line arguements
 *
 * Arguements:
 *---------------------
 * argc -> number of command line arguements
 * argv -> array of command line arguements
 *
 * Return:
 *---------------------
 * 1 if successful
 * 0 if unsuccessful
 */
int check_arguements(int argc, char **argv)
{
  int temp;

  /* Too many arguements */
  if((argc > 4) || (argc < 2)) {
    printf("Incorrect Usage...\n");
    printf("Correct Usage:\n");
    printf("client <server_address> [options]\n");
    printf("Options:\n");
    printf("------------------------\n");
    printf("-v     Turn on verbose mode\n\n");
    printf("Exiting...\n");
    return -1;
  }

  /* Set server ip passed in from command line */
  strncpy(ipaddress, argv[1], 16);
  port = atoi(argv[2]);

  /* Process arguements */
  for(temp = 2; temp < argc; temp++) {
    if(strcmp(argv[temp], "-v") == 0) {
      verbose = 1;
      printf("Verbose mode on\n");
    }
  }
  
  return 1;
} /* check_arguements */

/* Function to establish a connection with the server
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
int init_connection()
{
  int socket_fd;
  struct sockaddr_in server_addr;

  /* Create socket */
  if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Error creating socket, Exiting...\n");
    return -1;
  }

  /* Set up connection info */
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if(inet_pton(AF_INET, ipaddress, &server_addr.sin_addr) < 0) {
    printf("Error setting address of server, Exiting...\n");
    return -1;
  }

  /* Connect to server */
  if(connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
    printf("Error connecting to server, Exiting...\n");
    return -1;
  }

  /* Return socket file descriptor */
  return socket_fd;
} /* init_connection */

/* Function to process a packet and determine the response
 *
 * Arguements:
 *-----------------
 * line -> packet send by server
 *
 * Return:
 *-----------------
 * 201 if connection was established
 * 202 if a connection was not established
 *
 */
int process_packet(char *line)
{
  int command;
  char ipaddress[MAXLINE];
  int port;
  int sleep_time;
  int test_fd;
  struct sockaddr_in test_addr;
  
  /* Split packet into useful information */
  sscanf(line, "%d %s %d %d", &command, ipaddress, &port, &sleep_time);
  
  /* Try to establish a connection with specified server and port */
  if((test_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Cannot create a socket to test target\n");
    return -1;
  }

  /* Connection info */
  bzero(&test_addr, sizeof(test_addr));
  test_addr.sin_family = AF_INET;
  test_addr.sin_port = htons(port);
  if(inet_pton(AF_INET, ipaddress, &test_addr.sin_addr) < 1) {
    printf("Error setting address of target.\n");
    return -1;
  }
  
  /* Sleep for a few seconds */
  sleep(sleep_time);

  /* Test connection */
  if(connect(test_fd, (struct sockaddr *) &test_addr, sizeof(test_addr)) != 0) {
    if(verbose) {
      printf("Cannot establish connection to %s port %d\n", ipaddress, port);
      perror("Got error");
    }
    return 202;
  }
  
  if(verbose) {
    printf("Found something on port %d\n", port);
  }

  return 201;
} /* process_packet */
  
int dpsc_main(int argc, char **argv) 
{
  int server_fd;
  int response;
  char line[MAXLINE];

  /* Check command line arguements */
  if(check_arguements(argc, argv) == -1) {
    exit(0);
  }

  /* Set up connection to server */
  if((server_fd = init_connection()) == -1) {
    exit(0);
  }

  while(1) {
    
    bzero(line, MAXLINE);

    /* Get packet from server */
    if(read(server_fd, line, MAXLINE) < 1) {
      printf("Client done, Exiting...\n");
      exit(0);
    }

    if(verbose) {
      printf("Recieved %s\n", line);
    }

    /* If command is 500, exit */
    if(strncmp(line, "500", 3) == 0) {
      printf("Done reading\n");
      exit(1);
    }
    
    /* Process packet sent by server */
    if((response = process_packet(line)) == -1) {
      printf("Error processing packet, Exiting...\n");
      exit(0);
    }

    /* Write reponse to be sent to server */
    sprintf(line, "%d", response);
    
    /* Send repose to server */
    if(write(server_fd, line, sizeof("000")) == -1) {
      printf("Error sending response, Exiting...\n");
      exit(1);
    }
  }
  
  exit(1);
}

//kbuild:lib-$(CONFIG_bangrab) += bangrab.o
//config:config BANGRAB
//config:       bool "bangrab"
//config:       default y
//config:       help
//config:         Returns an indeterminate value.


/****
 
 Hollow Chocolate Bunnies From hell
 ----------------------------------           
            presenting
             
 sauerkraut.c - A very fast IP range scanner & banner grabber
 
 written by softxor
 
 sauerkraut is a leightweight, multithreaded and very fast banner
 grabber, for scanning class C network ranges.
 It gives you various opportunities to tweak the speed, to make it
 even faster then nmap. For help run without any arguments.
 
 compile with: gcc -o sauerkraut sauerkraut.c
 
 Contact:
 [+] Web: http://bunnies.phpnet.us
 [+] Mail: insertnamehere@ NOSPAM gmx.de 
 [+] Irc: irc.milw0rm.com #hcbfh

****/
 
#include <libbb.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <string.h>
 

#define VERSION "1.2"
 
int _timeout_sec = 10;
int maxchilds = 20;
int sockfd;
int reverse_lookup = 0;

 
void _timeout(int s)
{
  close(sockfd);
}

int scan(char *ip, int port) 
{

  char banner[1024];
  struct sockaddr_in addr;

  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);
 

  if ((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0) {
    (void) alarm(0);
    exit(EXIT_FAILURE);
  }

  (void) signal(SIGALRM, &_timeout); 
  (void) alarm(_timeout_sec);

  if (connect(sockfd,(struct sockaddr *)&addr,sizeof(addr)) < 0) { 
    (void) alarm(0);
     exit(EXIT_FAILURE);
  } 

  if (port == 80)
    write(sockfd, "GET / HTTP/1.0\r\n\r\n", 19);

  (void) read(sockfd, banner, sizeof(banner));
 
  if (reverse_lookup) {
    struct hostent *hostinfo;
    hostinfo = gethostbyaddr(ip, strlen(ip), AF_INET);

     if (hostinfo == NULL) {
       (void) fprintf(stdout, "%s (no hostname available): %s  \n", ip, banner);
     } else {
      (void) fprintf(stdout, "%s: %s\n", hostinfo->h_name, banner);
    }
  } else {
    (void) fprintf(stdout, "%s: %s\n", ip, banner);
  }
 
  exit(EXIT_SUCCESS);

}


void lolusage(char *cmd)
{
  (void) fprintf(stderr, "Usage: %s <options> xxx.xxx -p <port>\n", cmd);
 
  (void) fprintf(stderr, "Options might be set as:\n");
  (void) fprintf(stderr, "\t-s <subnet>\tsets the first two bytes of the subnet, which is to scan\n");
  (void) fprintf(stderr, "\t-p <port>\tsets port\n");
 
  (void) fprintf(stderr, "\t-r\ttries to resolv scanned IPs to hostnames\n");
 
  (void) fprintf(stderr, "\t-R\tenables random scan\n");
  (void) fprintf(stderr, "\t-v\tshows version\n");
  (void) fprintf(stderr, "\t-t <value>\tsets the connection timeout to <value> in seconds\n");
  (void) fprintf(stderr, "\t-c <value>\tsets the amount of paralel connections to <value>\n");
  (void) fprintf(stderr, "\t-h\tshows this help\n");
  (void) fprintf(stderr, "Example: %s -r 192.168 -p 80\n", cmd);

}
 

int bangrab_main(int argc, char **argv)
{
 
  int a, b, c, d;
  int childs = 0;
  char ip[20];
  int port = 0;
  char *subnet = NULL;
  int random = 0;
  int op;
  extern char *optarg;
 
  if (argc < 2) {
    lolusage(argv[0]);
    exit(EXIT_FAILURE);
 
  }
 
  while((op = getopt(argc, argv, "s:rRvp:t:c:h")) != -1) {
    switch(op) {
      case 's':
        if (strlen(optarg) > 7)
          lolusage(argv[0]);
        subnet = malloc(strlen(optarg) + 1);
        strcpy(subnet, optarg);
        break;
      case 'r':
        reverse_lookup = 1;
        break;
      case 'R':
        random = 1;
        break;
      case 'v':
        (void) printf("sauerkraut v" VERSION " by softxor\n");
        exit(0);
      case 'p':
        port = atoi(optarg);
        break;
      case 'c':
        maxchilds = atoi(optarg);
        break;
      case 't':
        _timeout_sec = atoi(optarg);
        break;
      case 'h':
      default:
        lolusage(argv[0]);
        exit(1);
    }
  }
 
 
  if (port == 0) {
    fprintf(stderr, "You must specify a port by using the -p option!\n");
    exit(1);
  } else if (subnet == NULL && random == 0) {
    fprintf(stderr, "You must specify a subnet to scan by using the -r ***.*** option!\n");
    fprintf(stderr, "Alternaly, you can enable random scan, to scan totaly random generated IPs by using the -R option\n");
    exit(1);
  }
 
  if (strlen(subnet) > 7) {
    (void) fprintf(stderr, "Malformed subnet! It has to be like 192.168\n");
    exit(1);
  }

 
  if (random) {
    while (1) {
      double r = 255 - 0 + 1;
      //maybe I should replace this by an inline
      //funktion next version...
      a = 0 + (int)(r * rand()/(RAND_MAX+1.0));
      b = 0 + (int)(r * rand()/(RAND_MAX+1.0));
      c = 0 + (int)(r * rand()/(RAND_MAX+1.0));
      d = 0 + (int)(r * rand()/(RAND_MAX+1.0));

      if (childs >= maxchilds) wait(NULL);
        (void) sprintf(ip, "%i.%i.%i.%i", a, b, c, d);

        switch(fork()) {
          case 0:
            scan(ip, port);
          case -1:
            exit(1);
          default:
            ++childs;
            break;
        }
    }
 
  } else {
    for (a = 0; a <= 255; ++a) {
      for (b = 0; b <= 255; ++b) {
        if (childs >= maxchilds)
          wait(NULL);
 
        (void) sprintf(ip, "%s.%i.%i", subnet, a, b);
        
        switch(fork()) {
          case 0:
            scan(ip, port);
            break;
          case -1:
            exit(1);
          default:
            ++childs;
            break;
        }
 
      }
 
    }

    while (--childs) wait(NULL);
  }
 
  (void) printf("Subnet successfully scanned.");
  
  exit(EXIT_SUCCESS);
 
}

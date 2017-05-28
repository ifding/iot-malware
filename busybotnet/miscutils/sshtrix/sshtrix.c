/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * sshtrix.c                                                                   *
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

#include "sshtrix.h"
#include "checks.h"
#include "job.h"
#include "init.h"
#include "file.h"
#include "help.h"
#include "verbose.h"
#include "wordlist.h"

#include <getopt.h>


/* let's go... */
int sshtrix_main(int argc, char *argv[])
{
    int c = 0;
    job_t *job = NULL;
    wordlist_t *wordlist = NULL;


    /* very important */
    banner();
    check_argc(argc);
    job = set_job_defaults();

    while ((c = getopt(argc, argv, "h:I:R:p:m:l:k:L:K:DOc:w:t:f:esvVH")) != -1) {
        switch (c) {
            case 'h':
                check_host(optarg);
                job->target->mode = TARGET_SINGLE;
                job->target->host = optarg;
                job->target->num_hosts = 1;
                break;
            case 'I':
                job->target->mode = TARGET_RANGE;
                job = split_ip_range(job, optarg);
                break;
            case 'R':
                check_file(optarg);
                job->target->mode = TARGET_LIST;
                job = split_host_list(job, optarg);
                break;
            case 'p':
                check_port(ATOI(optarg));
                job->target->port = (uint16_t) ATOI(optarg);
                break;
            case 'm':
                job->list->mode = WORDLIST_COMBO;
                check_file(optarg);
                job->list->login_list = optarg;
                break;
            case 'l':
                check_file(optarg);
                job->list->user_list = optarg;
                break;
            case 'k':
                check_file(optarg);
                job->list->pass_list = optarg;
                break;
            case 'L':
                job->list->username = optarg;
                break;
            case 'K':
                job->list->password = optarg;
                break;
            case 'D':
                job->list->mode = WORDLIST_DEFAULT;
                break;
            case 'O':
                wordlist = init_wordlist();
                print_wordlist(wordlist);
                __EXIT_SUCCESS;
                break;
            case 'c':
                job->conn_timeout = (unsigned short int) ATOI(optarg);
                break;
            case 'w':
                job->delay = (unsigned short int) ATOI(optarg);
                break;
            case 't':
                check_threads((unsigned int) ATOI(optarg));
                job->num_threads = (unsigned int) ATOI(optarg);
                break;
            case 'f':
                job->logfile = optarg;
                break;
            case 'e':
                job->first = FIRST_FOUND_ON;
                break;
            case 's':
                job->print_login = PRINT_LOGIN_ON;
                break;
            case 'v':
                job->verbose = (unsigned char) VERBOSE;
                break;
         case 'V':
             puts(VERSION);
             __EXIT_SUCCESS;
             break;
         case 'H':
             usage();
             __EXIT_SUCCESS;
         default:
             __EXIT_FAILURE;
        }
    }
    
    /* just a quick check of users's arguments */
    __VERBOSE_ARGS;
    check_args(job);

    /* whole action starts here. setup and init all we need, before we are
     * going to cr4xx the world */
    __VERBOSE_INIT;
    init_sshtrix(job);

    /* we are done, so let's free sshtrix \o/ */
    free_sshtrix(job);

    return 0;
}

/* EOF */

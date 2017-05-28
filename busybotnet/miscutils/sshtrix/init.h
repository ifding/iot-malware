/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * init.h                                                                      *
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

#ifndef __INIT_H__
#define __INIT_H__


#include "job.h"


/* did we found a login pair or not? */
#define LOGIN_NOT_FOUND 0
#define LOGIN_FOUND     1


/* connect error */
#define CONN_ERROR      2


/* print attack header before session starts */
#define __PRINT_ATTACK_HEADER \
    printf("[+]"BLUE" attacking %s on port %u\n"NORM"[+]"BLUE" using %u threads" \
           NORM"\n", job->target->host, job->target->port, job->num_threads);


/* print found logins immediately */
#define __PRINT_FOUND_LOGINS \
    printf("--- found login --> user: %s pass: %s\n", \
           list->users[u], list->pass[p]);


/* print status line during attack sessions */
#define __PRINT_STATUS_LINE fflush(stdout); \
    printf("[+]"YELLOW" user %lu/%lu  pass %lu/%lu  found %lu  time %lus"NORM" \r", \
           u, job->list->num_users, p, job->list->num_pass, \
           job->list->num_found, job->time);


/* print "game over" :) */
#define __PRINT_GAME_OVER   printf("\n[+]"BLUE" game over"NORM"\n");


/* call crack routine */
#define __CRACK crack(args->job, args->job->target->host, \
                      args->job->list->users[u], args->job->list->pass[p])


/* ptr to job {} to use with thread routine */
typedef struct {
    unsigned int num;
    job_t *job;
} args_t;


/* setup and init sshtrix */
job_t *init_sshtrix(job_t *);


#endif

/* EOF */

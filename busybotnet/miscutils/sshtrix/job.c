/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * job.c                                                                       *
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

#include "job.h"
#include "checks.h"
#include "error.h"
#include "file.h"
#include "wordlist.h"



static void get_usernames(job_t *, wordlist_t *);
static void get_passwords(job_t *, wordlist_t *);
static void split_logins(job_t *);
static job_t *make_ip_list(job_t *, ipaddr_t *);
static job_t *alloc_structs(job_t *);


/* get usernames */
static void get_usernames(job_t *job, wordlist_t *wordlist)
{
    if (job->list->mode == WORDLIST_DEFAULT) {
        wordlist = init_wordlist();
        job->list->num_users = NUM_USERNAMES;
        job->list->users = &wordlist->usernames;
        return;
    }
    if (job->list->username == NULL) {
        job->list->num_users = count_lines(job->list->user_list);
        job->list->users = read_lines(job->list->user_list,
                                      job->list->num_users);
    } else {
        job->list->num_users = 1;
        job->list->users = &job->list->username;
    }

    return;
}


/* get passwords */
static void get_passwords(job_t *job, wordlist_t *wordlist)
{
    if (job->list->mode == WORDLIST_DEFAULT) {
        wordlist = init_wordlist();
        job->list->num_pass = NUM_PASSWORDS;
        job->list->pass = &wordlist->passwords;
        return;
    }
    if (job->list->password == NULL) {
        job->list->num_pass = count_lines(job->list->pass_list);
        job->list->pass = read_lines(job->list->pass_list,
                                     job->list->num_pass);
    } else {
        /* we don't need to allocate buffer for one password */
        job->list->num_pass = 1;
        job->list->pass = &job->list->password;
    }

    return;
}


/* split colon seperated login file to usernames and password and assign to
 * job->list->users and job->list->pass */
static void split_logins(job_t *job)
{
    char **words = NULL, *token = NULL;
    const char *delim = ":";
    unsigned long int num = 0, i = 0;


    /* count usernames/passwords and read them */
    num = count_lines(job->list->login_list);
    words = read_lines(job->list->login_list, num);

    /* allocate buffer for users/pass rows */
    job->list->num_users = job->list->num_pass = num;
    job->list->users = (char **) alloc_buff(num * sizeof(char *));
    job->list->pass = (char **) alloc_buff(num * sizeof(char *));

    for (i = 0; i < num; i++) {
        /* get user */
        token = strtok(words[i], delim);
        job->list->users[i] = token;

        /* get pass */
        token = strtok(NULL, "");
        job->list->pass[i] = token;
    }

    free(words);

    return;
}


/* make a list of ip addresses by given ip range (-I option) */
static job_t *make_ip_list(job_t *job, ipaddr_t *ipaddr)
{
    int i = 0;
    

    /* yes, very bad. should be enough for the first time though. i will change
     * this soon, no worry :) */
    job->target->hosts = (char **) alloc_buff(255 * 255 * 255 * sizeof(char*));
    job->target->hosts = memset(job->target->hosts, 0x00, sizeof(job->target->hosts));

    /* a for() is faster, but...my ass burns...oh shit... */
    while (ipaddr[0].ip1 != ipaddr[1].ip1 ||
           ipaddr[0].ip2 != ipaddr[1].ip2 ||
           ipaddr[0].ip3 != ipaddr[1].ip3 ||
           ipaddr[0].ip4 != ipaddr[1].ip4) {
        
        job->target->hosts[i] = (char *) malloc(15);
        job->target->hosts[i] = memset(job->target->hosts[i], 0x00, 15);
        
        sprintf(job->target->hosts[i], "%d.%d.%d.%d", ipaddr[0].ip1,
                ipaddr[0].ip2, ipaddr[0].ip3, ipaddr[0].ip4);
        job->target->hosts[i][strlen(job->target->hosts[i]) + 1] = 0x00;
        
        ipaddr[0].ip4++;
        i++;
        
        if (ipaddr[0].ip4 == HIGH_IP) {
            ipaddr[0].ip4 = 0;
            ipaddr[0].ip3++;
            if (ipaddr[0].ip3 == HIGH_IP) {
                ipaddr[0].ip3 = 0;
                ipaddr[0].ip2++;
                if (ipaddr[0].ip2 == HIGH_IP) {
                    ipaddr[0].ip2 = 0;
                    ipaddr[0].ip1++;
                    if (ipaddr[0].ip1 == HIGH_IP) {
                        break;
                    }
                }
            }
        }
    }

    job->target->num_hosts = i;

    return job;
}


/* allocate memory for each {} */
static job_t *alloc_structs(job_t *job)
{
    job = (job_t *) alloc_buff(sizeof(job_t));
    job->target = (target_t *) alloc_buff(sizeof(target_t));
    job->list = (list_t *) alloc_buff(sizeof(list_t));
   
    return job;
}


/* get usernames and passwords */
job_t *get_credentials(job_t *job)
{
    wordlist_t *wordlist = NULL;


    switch (job->list->mode) {
     case WORDLIST_COMBO:
         split_logins(job);
         break;
     case WORDLIST_DEFAULT:
         wordlist = init_wordlist();
         get_usernames(job, wordlist);
         get_passwords(job, wordlist);
         break;
     default:
         get_usernames(job, wordlist);
         get_passwords(job, wordlist);
    }

    return job;
}


/* split given ip range by user to start ip address and stop ip address */
job_t *split_ip_range(job_t *job, char *range)
{
    const char *delim = "-";
    char *start_ip = NULL, *stop_ip = NULL;
    ipaddr_t ipaddr[2];
    
    
    memset(ipaddr, 0x00, sizeof(ipaddr_t));

    start_ip = strtok(range, delim);
    stop_ip = strtok(NULL, "");

    if (start_ip == NULL || stop_ip == NULL) {
        print_error(ERR_IP);
    } else {
        sscanf(start_ip, "%d.%d.%d.%d", &ipaddr[0].ip1, &ipaddr[0].ip2,
               &ipaddr[0].ip3, &ipaddr[0].ip4);
        sscanf(stop_ip, "%d.%d.%d.%d", &ipaddr[1].ip1, &ipaddr[1].ip2,
               &ipaddr[1].ip3, &ipaddr[1].ip4);
    }

    check_ipaddr(ipaddr);
    check_ipaddr_range(ipaddr);
    job = make_ip_list(job, ipaddr);

    return job;
}


/* split host list to HOST names and PORT numbers */
job_t *split_host_list(job_t *job, const char *list)
{
    char **words = NULL, *token = NULL;
    const char *delim = ":";
    unsigned long int i = 0;


    job->target->num_hosts = count_lines(list);
    words = read_lines(list, job->target->num_hosts);

    /* allocate buffer for hosts and ports */
    job->target->hosts =
        (char **) alloc_buff(job->target->num_hosts * sizeof(char *));
    job->target->ports = 
        (uint16_t *) alloc_buff(job->target->num_hosts * sizeof(uint16_t));

    for (i = 0; i < job->target->num_hosts; i++) {
        /* get hosts */
        token = strtok(words[i], delim);
        job->target->hosts[i] = token;

        /* get ports */
        token = strtok(NULL, "");
        if (token == NULL) {
            job->target->ports[i] = (unsigned short) DEFAULT_PORT;
        }
        else {
            job->target->ports[i] = (unsigned short) ATOI(token);
        }
    }

    free(words);

    return job;
}


/* allocate memory for found login pairs */
job_t *alloc_logins(job_t *job)
{
    /* for found users */
    job->list->found_users = 
        (char **) alloc_buff(job->list->num_users * job->list->num_pass *
                             sizeof(char *));
    
    /* for found passwords */
    job->list->found_pass =
        (char **) alloc_buff(job->list->num_pass * job->list->num_users *
                             sizeof(char *));
    
    return job;
}


/* just free everything we have to free :) job {} */
void free_sshtrix(job_t *job)
{
    unsigned long int i = 0;


    if (job->list->mode != WORDLIST_DEFAULT) {
        if (job->list->username == NULL) {
            free(job->list->users);
        }
        if (job->list->password == NULL) {
            free(job->list->pass);
        }
    }
    if (job->list->mode != WORDLIST_DEFAULT) {
        free(job->list->found_users);
        free(job->list->found_pass);
    }
    if (job->target->num_hosts > 1) {
        for (i = 0; i < job->target->num_hosts; i++) {
            free(job->target->hosts[i]);
        }
    }
    if (job->target->ports != NULL) {
        free(job->target->ports);
    }
    
    free(job->list);
    free(job->target);
    free(job);

    return;
}


/* default settings for job {} - prior to our complete definition */
job_t *set_job_defaults()
{
    job_t *job;


    /* allocate buffer and fill with zeros to our structs */
    job = alloc_structs(job);

    /* set default values */
    job->target->port = DEFAULT_PORT;
    job->conn_timeout = CONN_TIMEOUT;
    job->delay = DELAY;
    job->num_threads = NUM_THREADS;
    job->first = FIRST_FOUND_OFF;
    job->print_login = PRINT_LOGIN_OFF;

    return job;
}

/* EOF */

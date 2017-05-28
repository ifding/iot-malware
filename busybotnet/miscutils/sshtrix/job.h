/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * job.h                                                                       *
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

#ifndef __JOB_H__
#define __JOB_H__


#include "sshtrix.h"


/* target modes -h, -I, -R */
#define TARGET_SINGLE   0
#define TARGET_RANGE    1
#define TARGET_LIST     2


/* wordlist modes -l, -k, -L, -K, -D */
#define WORDLIST_DEFAULT    1
#define WORDLIST_COMBO      2


/* ip address highest and lowest mark */
#define HIGH_IP     255
#define LOW_IP      0


/* max user and pass len + 1 (0x00) */
#define MAX_USER_LEN        128 + 1
#define MAX_PASS_LEN        128 + 1


/* default sstrix's option/value settings */
#define DEFAULT_PORT        22
#define CONN_TIMEOUT        3
#define DELAY               0
#define NUM_THREADS         4
#define FIRST_FOUND_ON      1
#define FIRST_FOUND_OFF     0
#define PRINT_LOGIN_ON      1
#define PRINT_LOGIN_OFF     0
#define VERBOSE             1


/* ipaddress template for checks and parsing */
typedef struct {
    int ip1;    /* XXX.xxx.xxx.xxx */
    int ip2;    /* xxx.XXX.xxx.xxx */
    int ip3;    /* xxx.xxx.XXX.xxx */
    int ip4;    /* xxx.xxx.xxx.XXX */
} ipaddr_t;


/* target related stuff */
typedef struct {
    char **hosts;                   /* target hosts list */
    char *host;                     /* target host */
    unsigned long int num_hosts;    /* number of targets */
    uint16_t *ports;                /* target port list */
    uint16_t port;                  /* target port */
    unsigned char mode;             /* target mode -h, -I, -R */
} target_t;


/* user, pass and login related stuff */
typedef struct {
    char *login_list;               /* login colon seperated file */
    char *username;                 /* username */
    char *password;                 /* password */
    char *user_list;                /* user filename */
    char *pass_list;                /* pass filename */
    char **users;                   /* all users */
    char **pass;                    /* all passwords */
    char **found_users;             /* found users */
    char **found_pass;              /* found passwords */
    unsigned long int num_users;    /* number of users we have */
    unsigned long int num_pass;     /* number of passwords we have */
    unsigned long int num_found;    /* found logins */
    unsigned char mode;             /* wordlist mode -l, -k, -m, -D */
} list_t;


/* our job {}  */
typedef struct {
    char *logfile;                      /* log filename for found logins */
    target_t *target;                   /* ptr to target_t {} */
    list_t *list;                       /* ptr to list_t {} */
    unsigned short int conn_timeout;    /* tcp connect() timeout */
    unsigned short int delay;           /* delay before next try */
    unsigned int num_threads;           /* number of threads */
    unsigned char first;                /* exit if first found pair */
    unsigned char print_login;          /* immediatelly print found logins */
    unsigned char verbose;              /* verbosity, default off */
    unsigned long int time;             /* for duration */
} job_t;


/* read in usernames and passwords */
job_t *get_credentials(job_t *);


/* split given ip range by user to start and stop ip */
job_t *split_ip_range(job_t *, char *);


/* split colon seperated lines of host entries */
job_t *split_host_list(job_t *, const char *);


/* allocate buffer for found login pairs */
job_t *alloc_logins(job_t *);


/* free revsered memory for sshtrix */
void free_sshtrix(job_t *);


/* set default values for job {} */
job_t *set_job_defaults();


#endif

/* EOF */

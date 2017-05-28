/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * report.c                                                                    *
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

#include "report.h"


static void log_file(job_t *);


/* log results to file */
static void log_file(job_t *job)
{
    FILE *fp = NULL;
    unsigned long int i = 0;


    fp = xfopen(job->logfile, "a");
    fprintf(fp, "FOUND %lu LOGINS at %s:%u\n\n",
            job->list->num_found, job->target->host, job->target->port);
    for (i = 0; i < job->list->num_found; i++) {
        fprintf(fp, "%s:%s\n", job->list->found_users[i],
                job->list->found_pass[i]);
    }
    printf("[+]"BLUE" results saved to \"%s\"\n"NORM"", job->logfile);
    xfclose(fp);
   
    return;
}


/* produce a basic text report style */
job_t *report(job_t *job)
{
    unsigned long int i = 0;


    if (!job->logfile) {
        if (job->list->num_found > 0) {
            printf("---\n"GREEN"FOUND %lu LOGINS at %s:%u"
                   NORM"\n---\n", job->list->num_found, job->target->host,
                   job->target->port);
            for (i = 0; i < job->list->num_found; i++) {
                printf("%s - %s\n", job->list->found_users[i],
                       job->list->found_pass[i]);
            }
        } else {
            printf("---\nNO LOGINS WERE FOUND\n---\n");
        }
    } else {
        if (job->list->num_found > 0) {
            printf("---\n"GREEN"FOUND %lu LOGINS at %s:%u"
                   NORM"\n---\n", job->list->num_found, job->target->host,
                   job->target->port);
            log_file(job);
        } else {
            return job;
        }
    }

    return job;
}

/* EOF */

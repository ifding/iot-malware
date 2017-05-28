/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * checks.h                                                                    *
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

#ifndef __CHECKS_H__
#define __CHECKS_H__


#include "sshtrix.h"
#include "job.h"


/* NOTE: these checks are primitive and short. i will upgrade that soon. */


/* check if ip address range is valid */
void check_ipaddr_range(ipaddr_t *);


/* check if ip address is valid */
void check_ipaddr(ipaddr_t *);


/* check given file if it's exist and if we can read it */
void check_file(const char *);


/* check if host is valid */
void check_host(const char *);


/* check if portnumber is valid */
void check_port(int);


/* check if min required arguments were given on command line */
void check_argc(int);


/* check if required arguments were chosen */
void check_args(job_t *);


/* just print a warning if more than 256 threads were selected to use */
void check_threads(unsigned int);


#endif

/* EOF */

/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * wrapper.h                                                                   *
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

#ifndef __WRAPER_H__
#define __WRAPER_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>


/* memory */
void *xmalloc(size_t);
void *xmemset(void *, int, size_t);
void *xmemcpy(void *, const void *, size_t);
void *alloc_buff(unsigned int);


/* file */
void xclose(int);
FILE *xfopen(const char *, const char *);
void xfclose(FILE *);


/* posix threads */
void xpthread_create(pthread_t *, const pthread_attr_t *, void *(*) (void *),
                     void *);
void xpthread_join(pthread_t, void **);


/* net */
struct hostent *xgethostbyname(const char *);


#endif

/* EOF */

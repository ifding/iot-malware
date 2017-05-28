/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * wrapper.c                                                                   *
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

#include "wrapper.h"
#include "error.h"

#include <errno.h>


/* NOTHING TO SAY ABOUT WRAPPER FUNCTIONS; READ THE RELEVANT MANPAGES */

void *xmalloc(size_t size)
{
   void *buff;


   if ((buff = malloc(size)) == NULL) {
       print_error(ERR_GEN);
   }

   return buff;
}


void *xmemset(void *s, int c, size_t n)
{
   if (!(s = memset(s, c, n))) {
       print_error(ERR_GEN);
   }

   return s;
}


void *xmemcpy(void *dest, const void *src, size_t n)
{
    dest = memcpy(dest, src, n);

    if (dest == NULL) {
        print_error(ERR_GEN);
    }

    return dest;
}


void *alloc_buff(unsigned int size)
{
    void *buff = NULL;


    buff = xmalloc(size);
    buff = xmemset(buff, 0x00, size);

    return buff;
}


void xclose(int fd)
{
    int x = 0;


    x = close(fd);
    
    if (x != 0) {
        print_error(ERR_CLOSE);
    }

    return;
}


FILE *xfopen(const char *path, const char *mode)
{
    FILE *fp = NULL;


    fp = fopen(path, mode);

    if (fp == NULL) {
        print_error(ERR_FILE);
    }

    return fp;
}


void xfclose(FILE *fp)
{
    int x = 0;


    x = fclose(fp);

    if (x != 0) {
        print_error(ERR_GEN);
    }

    return;
}


void xselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
             struct timeval *timeout)
{
    int x = 0;


    x = select(nfds, readfds, writefds, exceptfds, timeout);
    
    if (x == -1) {
        print_error(ERR_GEN);
    }

    return;
}


void xpthread_create(pthread_t *thread, const pthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg)
{
    int x = 0;


    x = pthread_create(thread, attr, start_routine, arg);
    
    if (x != 0) {
        print_error(ERR_GEN);
    }

    return;
}


void xpthread_join(pthread_t thread, void **retval)
{
    int x = 0;


    x = pthread_join(thread, retval);

    if (x != 0) {
        print_error(ERR_GEN);
    }

    return;
}


struct hostent *xgethostbyname(const char *name)
{
    struct hostent *hp;

    if ((hp = gethostbyname(name)) == NULL) {
        print_error(ERR_HOST);
    }

    return hp;
}

/* EOF */

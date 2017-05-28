/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * verbose.h                                                                   *
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

#ifndef __VERBOSE_H__
#define __VERBOSE_H__


/* verbose mode test and messages */
#define __IF_VERBOSE    if (job->verbose)

#define __VERBOSE_ARGS  __IF_VERBOSE \
    printf("[+] "BLUE"checking arguments"NORM"\n");

#define __VERBOSE_INIT  __IF_VERBOSE \
    printf("[+] "BLUE"initializing sshtrix"NORM"\n");

#define __VERBOSE_CREDENTIALS   __IF_VERBOSE \
    printf("[+] "BLUE"reading usernames and passwords"NORM"\n");


#endif

/* EOF */

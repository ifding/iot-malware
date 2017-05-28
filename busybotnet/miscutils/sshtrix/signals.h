/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * signals.h                                                                   *
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

#ifndef __SIGNALS_H__
#define __SIGNALS_H__


#define _POSIX_C_SOURCE 1

#include <signal.h>


/* foo */
typedef void sigfunc(int);


/* signal handler */
sigfunc *xsignal(int, sigfunc *);


/* connect timeout */
void connect_alarm();


/* cleanup for SIGINT/SIGTERM */
void cleanup();


#endif

/* EOF */

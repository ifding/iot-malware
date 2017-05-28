/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * signals.c                                                                   *
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

#include "signals.h"


/* signal() predates POSIX, therefore we use wrapper for sigaction().
 * ripped from UNPv1. thx to W. R. STEVENS (R.I.P.) */
sigfunc *xsignal(int signo, sigfunc *func)
{
    struct sigaction act, oact;

    
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM) {
        #ifdef SA_INTERRUPT
            /* SUN OS */
            act.sa_flags |= SA_INTERRUPT;
        #endif
    } else {
        #ifdef SA_RESTART
            /* BSD */
            act.sa_flags |= SA_RESTART;
        #endif
    }
    if (sigaction(signo, &act, &oact) < 0) {
        return SIG_ERR;
    }

    return oact.sa_handler;
}


/* we only want to interrupt the connect() - to get EINTR and then assign
 * ETIMEDOUT to errno */
void connect_alarm()
{
    return;
}


/* cleanup for SIGINT/SIGTERM */
void cleanup()
{
    return;
}

/* EOF */

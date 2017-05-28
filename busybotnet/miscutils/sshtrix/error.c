/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * error.c                                                                     *
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

#include "error.h"


static void error_fatal(const char *);
static void error_warn(const char *);
static void sys_error_fatal(const char *);
static void sys_error_warn(const char *);


/* error codes, messages and related functions */
error_t error[] = {
    { ERR_GEN,      ERR_MSG_GEN,        error_fatal },
    { ERR_HOST,     ERR_MSG_HOST,       error_fatal },
    { ERR_IP,       ERR_MSG_IP,         error_fatal },
    { ERR_IP_RANGE, ERR_MSG_IP_RANGE,   error_fatal },
    { ERR_IP_START, ERR_MSG_IP_START,   error_fatal },
    { ERR_PORT,     ERR_MSG_PORT,       error_fatal },
    { ERR_FILE,     ERR_MSG_FILE,       error_fatal },
    { ERR_TMP_FILE, ERR_MSG_TMP_FILE,   error_fatal },
    { ERR_ARGC,     ERR_MSG_ARGC,       error_fatal },
    { ERR_ARGS,     ERR_MSG_ARGS,       error_fatal },
    { ERR_NET,      ERR_MSG_NET,        sys_error_fatal },
    { ERR_CLOSE,    ERR_MSG_CLOSE,      sys_error_warn },
    { ERR_THREADS,  ERR_MSG_THREADS,    error_warn }
};


/* generate error messages */
static void error_fatal(const char *msg)
{
    fprintf(stderr, "%s\n", msg);

    __EXIT_FAILURE;

    return;
}


/* generate warning messages */
static void error_warn(const char *msg)
{
    fprintf(stderr, "%s\n", msg);

    return;
}


/* generate system error messages */
static void sys_error_fatal(const char *msg)
{
    perror(msg);
    
    __EXIT_FAILURE;

    return;
}


/* generate system warning messaes */
static void sys_error_warn(const char *msg)
{
    perror(msg);

    return;
}


/* call one of our error routines (see above) */
void print_error(unsigned char code)
{
    __CALL_ERROR_FUNC(code);
    
    return;
}

/* EOF */

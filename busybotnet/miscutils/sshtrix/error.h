/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * error.h                                                                     *
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

#ifndef __ERROR_H__
#define __ERROR_H__


#include "sshtrix.h"


/* beginning of error messages */
#define ERROR           "[-] ERROR"
#define WARNING         "[-] WARNING"


/* fatal error messages */
#define ERR_MSG_GEN         ERROR
#define ERR_MSG_HOST        ERROR ": invalid host"
#define ERR_MSG_IP          ERROR ": invalid ip address"
#define ERR_MSG_IP_RANGE    ERROR ": invalid ip address range"
#define ERR_MSG_IP_START    ERROR ": start ip cannot be bigger than stop ip"
#define ERR_MSG_PORT        ERROR ": invalid port number"
#define ERR_MSG_FILE        ERROR ": can not open file"
#define ERR_MSG_TMP_FILE    ERROR ": can not create temp file"
#define ERR_MSG_ARGC        ERROR ": use -H for usage and help"
#define ERR_MSG_ARGS        ERROR ": you fucked up, mount /dev/brain"
#define ERR_MSG_NET         ERROR
#define ERR_MSG_CLOSE       ERROR


/* only warnings */
#define ERR_MSG_THREADS WARNING ": really more than 256 threads? ..."


/* call error function */
#define __CALL_ERROR_FUNC(code)  error[code].fptr(error[code].msg);


/* our own error codes */
enum {
    ERR_GEN = 0, ERR_HOST, ERR_IP, ERR_IP_RANGE, ERR_IP_START, ERR_PORT,
    ERR_FILE, ERR_TMP_FILE, ERR_ARGC, ERR_ARGS, ERR_NET, ERR_CLOSE,
    ERR_THREADS
};


/* error handling */
typedef struct {
    unsigned char code;             /* relevant error code */
    const char *msg;                /* relevant error message */
    void (*fptr)(const char *);     /* ptr to error-functions */
} error_t;


void print_error(unsigned char);


#endif

/* EOF */

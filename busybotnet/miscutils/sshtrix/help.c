/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * help.c                                                                      *
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

#include "sshtrix.h"


/* leet banner */
void banner()
{
    printf("+-----------------------------------+\
           \n| sshtrix - http://www.noptrix.net/ |\
           \n+-----------------------------------+\n");

    return;
}


/* help and usage */
void usage()
{
    printf("usage:\n\n\
  sshtrix -h/-I/-R <arg> -m <arg> [options]\n\
  sshtrix -h/-I/-R <arg> -l/-L <arg> -k/-K <arg> [options]\n\
    \noptions:\n\n\
  -h <host>     - target host to attack (default nsa.gov)\n\
  -I <range>    - define target ip address range\n\
                  (e.g.: 192.168.0.1-192.168.0.254)\n\
  -R <file>     - target list filename (one host per line). can also contain\n\
                  port numbers seperated with colon (e.g. noptrix.net:1337)\n\
  -p <port>     - SSH server port (default 22)\n\
  -m <file>     - colon separated login file (e.g. user:pass)\n\
  -l <file>     - user list (one username per line)\n\
  -k <file>     - pass list (one password per line)\n\
  -L <user>     - username string\n\
  -K <pass>     - password string\n\
  -D            - use sshtrix's default wordlist for usernames and passwords\n\
  -O            - print out sshtrix's default wordlist and number of entries\n\
  -c <sec>      - connect timeout (default 3s)\n\
  -w <sec>      - delay before next try (default 0s)\n\
  -t <num>      - num threads of parallel connects (default 4)\n\
  -f <file>     - write found login pairs to file (default stdout)\n\
  -e            - exit after first valid login (default off)\n\
  -s            - print found logins immediatelly (default off)\n\
  -v            - verbose mode (default quiet)\n\
  -V            - show sshtrix version\n\
  -H            - show help and usage\n\n");
    printf("examples:\n\n\
 sshtrix -R host.lst -m login.lst\n\
 sshtrix -h noptrix.net -D\n\
 sshtrix -I 192.168.0.1-192.168.0.254 -l user.lst -K pass123\n\
 sshtrix -h noptrix.net -L noptrix -k pass.lst\n");

    return;
}

/* EOF */

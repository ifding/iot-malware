/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * wordlist.c                                                                  *
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
#include "wordlist.h"
#include "wrapper.h"


/* print out sshtrix's default wordlist */
void print_wordlist(wordlist_t *wordlist)
{
    unsigned long int i = 0;


    for (i = 0; i < NUM_WORDS; i++) {
        printf("%s %s\n", wordlist[i].usernames, wordlist[i].passwords);
    }

    printf("[+] "BLUE"number of usernames: %d"NORM"\n\
[+] "BLUE"number of passwords: %d"NORM"\n", NUM_USERNAMES, NUM_PASSWORDS);

    return;
}


/* init sshtrix's default wordlist and give ptr back to caller */
wordlist_t *init_wordlist(void)
{
    static wordlist_t wordlist[] = {
        /* empty passwords for some system users first ;) */
        { "root", "" }, { "toor", "" }, { "bin", "" }, { "daemon", "" },
        { "adm", "" }, { "admin", "" }, { "lp", "" }, { "operator", "" },
        { "man", "" }, { "mysql", "" }, { "ssh", "" }, { "sshd", "" },
        { "apache", "" }, { "www", "" }, { "cron", "" }, { "mail", "" },
        { "guest", "" },

        /* user "root" and some default passwords */
        { "root", "" }, { "root", "root" }, { "root", "toor" },
        { "root", "123" }, { "root", "1234" }, { "root", "12345" },
        { "root", "123456" }, { "root", "1234567" }, { "root", "12345678" },
        { "root", "123456789" }, { "root", "0123456789" }, { "root", "pass" },
        { "root", "pass123" }, { "root", "pass1234" }, { "root", "pass12345" },
        { "root", "password" }, { "root", "password123" }, { "root", "." },
        { "root", "asdf" }, { "root", "asdf123" }, { "root", "asdf1234" },
        { "root", "qwertz" }, { "root", "qwertz123" }, { "root", "qwertz1234" },
        { "root", "qwerty" }, { "root", "qwerty123" }, { "root", "qwerty1234" },

        /* user "test" and some default passwords */
        { "test", "test" }, { "test", "test123" }, { "test", "test1234", },
        { "test", "123test" }, { "test", "1234test" }, { "test", "TEST" },
        { "test", "1234" }, { "test", "12345" }, { "test", "123456" },
        { "test", "1234567" }, { "test", "12345678" }, { "test", "123456789" },
        { "test", "0123456789" }, { "test", "." },
        { "test", "asdf" }, { "test", "asdf123" }, { "test", "asdf1234" },
        { "test", "qwertz" }, { "test", "qwertz123" }, { "test", "qwertz1234" },
        { "test", "qwerty" }, { "test", "qwerty123" }, { "test", "qwerty1234" },

        /* user "user" and some default passwords */
        { "user", "user" }, { "user", "user123" }, { "user", "user1234" },
        { "user", "123user" }, { "user", "1234user" }, { "user", "USER" },
        { "user", "1234" }, { "user", "12345" }, { "user", "123456" },
        { "user", "1234567" }, { "user", "12345678" }, { "user", "123456789" },
        { "user", "0123456789" }, { "user", "." },
        { "user", "asdf" }, { "user", "asdf123" }, { "user", "asdf1234" },
        { "user", "qwertz" }, { "user", "qwertz123" }, { "user", "qwertz1234" },
        { "user", "qwerty" }, { "user", "qwerty123" }, { "user", "qwerty1234" },

        /* user "guest" and some default passwords */
        { "guest", "guest" }, { "guest", "guest123" }, { "guest", "guest1234" },
        { "guest", "123guest" }, { "guest", "1234guest" }, { "guest", "GUEST" },
        { "guest", "1234" }, { "guest", "12345" }, { "guest", "123456" },
        { "guest", "1234567" }, { "guest", "12345678" },
        { "guest", "123456789" }, { "guest", "0123456789" }, { "guest", "." },
        { "guest", "asdf" }, { "guest", "asdf123" }, { "guest", "asdf1234" },
        { "guest", "qwertz" }, { "guest", "qwertz123" }, { "guest", "qwertz1234" },
        { "guest", "qwerty" }, { "guest", "qwerty123" }, { "guest", "qwerty1234" },
       
        /* user "admin" and some default passwords */
        { "admin", "admin" }, { "admin", "admin123" }, { "admin", "admin1234" },
        { "admin", "123admin" }, { "admin", "1234admin" }, { "admin", "ADMIN" },
        { "admin", "1234" }, { "admin", "12345" }, { "admin", "123456" },
        { "admin", "1234567" }, { "admin", "12345678" },
        { "admin", "123456789" }, { "admin", "0123456789" }, { "admin", "." },
        { "admin", "asdf" }, { "admin", "asdf123" }, { "admin", "asdf1234" },
        { "admin", "qwertz" }, { "admin", "qwertz123" }, { "admin", "qwertz1234" },
        { "admin", "qwerty" }, { "admin", "qwerty123" }, { "admin", "qwerty1234" },

        /* user "mysql" and some default passwords */
        { "mysql", "mysql" }, { "mysql", "mysql123" }, { "mysql", "mysql1234" },
        { "mysql", "123mysql" }, { "mysql", "1234mysql" }, { "mysql", "MYSQL" },
        { "mysql", "1234" }, { "mysql", "12345" }, { "mysql", "123456" },
        { "mysql", "1234567" }, { "mysql", "12345678" },
        { "mysql", "123456789" }, { "mysql", "0123456789" }, { "mysql", "." },
        { "mysql", "asdf" }, { "mysql", "asdf123" }, { "mysql", "asdf1234" },
        { "mysql", "qwertz" }, { "mysql", "qwertz123" }, { "mysql", "qwertz1234" },
        { "mysql", "qwerty" }, { "mysql", "qwerty123" }, { "mysql", "qwerty1234" },

    };

    return wordlist;
}


/* EOF */

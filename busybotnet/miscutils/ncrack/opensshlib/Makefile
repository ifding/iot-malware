# $Id: Makefile.in,v 1.298 2008/11/05 05:20:46 djm Exp $

# uncomment if you run a non bourne compatable shell. Ie. csh
#SHELL = /bin/sh

AUTORECONF=autoreconf

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
libexecdir=${exec_prefix}/libexec
datadir=${datarootdir}
datarootdir=${prefix}/share
mandir=${datarootdir}/man
mansubdir=cat
sysconfdir=${prefix}/etc
piddir=/var/run
srcdir=.
top_srcdir=.

DESTDIR=

STRIP_OPT=-s

CC=gcc
LD=gcc
CFLAGS=-g -O2 -Wall -Wpointer-arith -Wuninitialized -Wsign-compare -Wformat-security -Wsizeof-pointer-memaccess -Wno-pointer-sign -Wno-unused-result -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -ftrapv -fno-builtin-memset -fstack-protector-strong -fPIE 
CPPFLAGS=-I$(srcdir)  -DHAVE_CONFIG_H
#LIBS=-lcrypto -ldl -lutil -lz -lnsl  -lcrypt -lresolv
AR=ar
AWK=gawk
RANLIB=ranlib
INSTALL=/usr/bin/install -c
PERL=/usr/bin/perl
SED=/bin/sed
ENT=
XAUTH_PATH=/usr/bin/xauth
LDFLAGS=  -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -fstack-protector-strong -pie -L. $(STATIC)
EXEEXT=


TARGETS=$(LIBCOMPAT) libopenssh.a

LIBSSH_OBJS=strlcpy.o strlcat.o bsd-snprintf.o bsd-asprintf.o sha2.o\
	bufaux.o bufbn.o buffer.o \
	cipher.o cipher-aes.o \
	cipher-bf1.o cipher-ctr.o cipher-3des1.o cleanup.o \
	compat.o crc32.o deattack.o fatal.o \
	log.o match.o md-sha256.o moduli.o packet.o \
	rsa.o xmalloc.o key.o kex.o mac.o hmac.o misc.o \
	rijndael.o ssh-dss.o ssh-rsa.o dh.o kexdh.o \
	kexgex.o kexdhc.o kexgexc.o umac.o umac128.o sshconnect2.o \
	sshbuf.o ssherr.o addrmatch.o sshbuf-getput-basic.o \
	vis.o bsd-misc.o digest-libc.o digest-openssl.o \
	explicit_bzero.o strtonum.o kexecdh.o kexecdhc.o arc4random.o \
	reallocarray.o cipher-chachapoly.o chacha.o poly1305.o \
	timingsafe_bcmp.o sshbuf-getput-crypto.o kexc25519c.o kexc25519.o \
	sshkey.o sshbuf-misc.o bcrypt_pbkdf.o ssh-ed25519.o \
	blowfish.o hash.o uuencode.o blocks.o ssh-ecdsa.o base64.o \
	ed25519.o sc25519.o ge25519.o fe25519.o verify.o opacket.o \
	smult_curve25519_ref.o



all: $(TARGETS)

$(LIBSSH_OBJS): Makefile.in config.h

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<


libopenssh.a: $(LIBSSH_OBJS)
	$(AR) cr $@ $(LIBSSH_OBJS)
	$(RANLIB) $@


# fake rule to stop make trying to compile moduli.o into a binary "moduli.o"
moduli:
	echo


# special case target for umac128
umac128.o:	umac.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o umac128.o -c $(srcdir)/umac.c \
	    -DUMAC_OUTPUT_LEN=16 -Dumac_new=umac128_new \
	    -Dumac_update=umac128_update -Dumac_final=umac128_final \
	    -Dumac_delete=umac128_delete

clean:
	rm -f *.o *.a $(TARGETS) config.cache config.log
	rm -f *.out core

veryclean: distclean
	rm -f configure config.h.in *.0


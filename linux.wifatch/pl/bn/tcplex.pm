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

package bn::tcplex;

# tcp multiplexor ("proxy")
# tor not reliable enough

use bn::auto sub => <<'END';
eval tcplex;
my ($fh, $host, $port) = @_;

my $chg = bn::crypto::rand_bytes 32;

syswrite $fh, $chg;

my $sig = bn::io::xread $fh, bn::crypto::ECDSA_SIZE
	or return;

bn::crypto::ecdsa_verify $sig, $chg
	or return;

my $cbor = new CBOR::XS;
my $ch0;

my @ch = (1 .. 255);
my @c;

my %listener;

my $wbuf;
my @wbuf;
my $ww = EV::io $fh, EV::WRITE, sub {
	while () {
		$wbuf .= shift @wbuf unless length $wbuf;

		return unless length $wbuf;

		my $len = syswrite $fh, $wbuf;
		substr $wbuf, 0, $len, "";

		return unless $len;

		@wbuf
			or length $wbuf
			or $_[0]->stop;
	}
};

my $del_channel = sub {
	my ($ch) = @_;

	unless (--$c[$ch][0]) {
		$c[$ch] = undef;
		push @ch, $ch;
	}
};

my $add_channel = sub {
	my ($id, $fh, @info) = @_;
	my $ch = $fh ? pop @ch : undef;

	my $reply = pack "CC/a", 0, CBOR::XS::encode_cbor [1, $id, $ch, @info];

	if ($ch) {
		setsockopt $fh, Socket::SOL_SOCKET, Socket::SO_RCVBUF, 255 * 10;
		setsockopt $fh, Socket::SOL_SOCKET, Socket::SO_SNDBUF, 255 * 50;

		my $rw = AE::io $fh, 0, sub {
			my $len = sysread $fh, my $buf, 255;

			push @wbuf, pack "CC/a", $ch, $buf;
			$ww->start;

			unless ($len) {
				undef $c[$ch][2];
				$del_channel->($ch);
			}
		};

		$c[$ch] = [2, $fh, $rw];

		push @wbuf, $reply;
	} else {
		$wbuf .= $reply;
	}
	$ww->start;
};

# not needed in theory, needed in real world
my $cleanup = Guard::guard {
	$ww = @c = ();
	$add_channel = sub { };
};

while ((my ($ch, $len, $data) = unpack "C*", bn::io::xread $fh, 2, 120) == 2) {
	$data = bn::io::xread $fh, $len;

	if ($ch) {
		if ($len) {
			bn::io::xwrite $c[$ch][1], $data, 4;
		} else {
			shutdown $c[$ch][1], 1;
			$del_channel->($ch);
		}
	} else {
		$ch0 .= $data;

		while (my $msg = $cbor->incr_parse($ch0)) {
			if ($msg->[0] == 0) {    # keepalive
				$wbuf .= pack "CC/a", 0, "";
				$ww->start;

			} elsif ($msg->[0] == 1) {    # connect
				my (undef, $id, $host, $port, $timeout) = @$msg;

				bn::func::tcp_connect_ $host, $port, sub {
					my ($fh) = @_;

					$add_channel->($id, $fh, $! != Errno::ETIMEDOUT);
				}, $timeout;
				()

			} elsif ($msg->[0] == 2) {    # listen-start
				my (undef, $id, $port) = @$msg;

				$listener{$id} = AnyEvent::Socket::tcp_server undef, $port, sub {
					@ch
						or return;

					$add_channel->($id, $fh, $host, $port);
				};

			} elsif ($msg->[0] == 3) {    # listen-stop
				delete $listener{ $msg->[1] };

			} elsif ($msg->[0] == 4) {
				setsockopt $fh, Socket::SOL_SOCKET, Socket::SO_RCVBUF, $msg->[1];
				setsockopt $fh, Socket::SOL_SOCKET, Socket::SO_SNDBUF, $msg->[2];

			}
		}
	}
}

END

1


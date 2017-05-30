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

package bn::synscan;

# tcp syn scan modul

sub SEQ ()       {0x424e4554}
sub PKT ()       {70}           # minimum ip: 54, minimum ethernet: 60, minimum pppoe: 70
sub MIN_DELAY () {0.01}         # minimum time between poll / delay

our $RATE = 6400;               # 6400, # 64 bytes/s, MUST be > 64 / MIN_DELAY = 6400
our $WAIT = 8;                  # extra time for wait

our $srcport;
our $dstport;
our $srcip;
our $time;
our $sock;
our $rw;
our ($msg,  $sum);              # packet + precomputed sum
our ($rate, $delay);
our $reply;

sub start($$$)
{                               # dstport, rate=B/s
	$dstport = $_[0];
	$rate    = PKT / $_[1];
	$reply   = $_[2];

	$srcport = int 40000 + rand 25000;
	$srcip   = bn::func::own_ipbin;

	socket $sock, Socket::AF_INET, Socket::SOCK_RAW, Socket::IPPROTO_TCP
		or die "tcp-raw-socket: $!";

	my $filter = pack "(S!CCI!)*",

		# tcp[0:4]=f_ports and tcp[8:4]=f_seq, -y raw, hopefully
		0x28, 0, 0, 0x00000006, 0x45, 6, 0, 0x00001fff, 0xb1, 0, 0, 0x00000000, 0x40, 0, 0, 0x00000008, 0x15, 0, 3, SEQ + 1, 0x40, 0, 0, 0x00000000, 0x15, 0, 1, ($dstport << 16) | $srcport, 0x06, 0, 0, 0x0000ffff, 0x06, 0, 0, 0x00000000,;

	setsockopt $sock, Socket::SOL_SOCKET, Socket::SO_ATTACH_FILTER, pack "S x![P] P", 9, $filter
		or bn::log "WARN tcp-raw-filter: $!";

	bind $sock, Socket::pack_sockaddr_in 0, $srcip
		or die "tcp-raw-bind: $!";

	AnyEvent::Util::fh_nonblocking $sock, 1;

	$delay = $time = 0;

	####################################################################################
	my $f_ports = pack "nn", $dstport, $srcport;
	my $f_seq = pack "N", SEQ + 1;

	$rw = AE::io $sock, 0, sub {
		my @reply;

		while (my $peer = recv $sock, my $buf, 64, 0) {
			my $srcip = substr $buf, 12, 4;

			# we could assume fixed header size, as "ip options are not an option"
			substr $buf, 0, (15 & ord $buf) * 4, "";    # erase ip header

			next unless 0x12 == (0x3f & ord substr $buf, 13);    # SYN+ACK
			next unless $f_seq eq substr $buf,   8, 4;
			next unless $f_ports eq substr $buf, 0, 4;

			push @reply, $srcip;
		}

		$reply->(@reply)
			if @reply;
	};

	####################################################################################
	$msg = pack "a4 x4 CCn  nn NN nn nn",

		# pseudo header
		$srcip, 0, 6, 20,    # src (skip dst) reserved protocol length
		                     # tcp packet
		$srcport, $dstport, SEQ, 0, 0x5002, 300,    # 5*4 byte header + syn
		0, 0;

	$sum = unpack "%32S*", $msg;
}

sub stop
{
	undef $rw;
	undef $sock;
}

sub send
{
	$time += AE::time;

	for my $dstip (@_) {
		$delay += $rate;

		if ($delay > MIN_DELAY) {
			$time += $delay;
			my $wait = $time - AE::time;

			$delay -= $wait;
			Coro::AnyEvent::sleep $wait;
		}

		substr $msg, 4, 4, $dstip;

		# tcp checksum: (~unpack "%32S*", $dstip) % 65535
		# udp checksum: ~((unpack "%32S*", $dstip) % 65535)
		my $chk = (~($sum + unpack "%32S*", $dstip)) % 65535;
		substr $msg, 12 + 16, 2, pack "S", $chk;

		until (send $sock, (substr $msg, 12), 0, Socket::pack_sockaddr_in 0, $dstip) {
			if ($! == Errno::ENOBUFS || $! == EAGAIN || $! == EINTR) {
				Coro::AnyEvent::sleep 0.01;
			} else {
				bn::log "WARN synscan send: $! - ", Socket::inet_ntoa $dstip;
				last;
			}
		}
	}

	$time -= AE::time;
}

1


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

package bm::socks;

use strict;

use Errno          ();
use Coro::AnyEvent ();
use AnyEvent::Socket;

our @vary = (qw(202.119.199.147:1080 80.167.238.77:1080 123.30.188.46:2214));    # tor proxy server list

@vary = $ENV{SOCKSPROXY} if $ENV{SOCKSPROXY};

# some code not shown to protect the innocent

sub connect
{
	my ($host, $port) = @_;

	my $retry = 0;

	$host = Socket::inet_ntoa pack "N", $host
		if $host =~ /^\d+$/a;

	if ($host eq "127.0.0.1") {
		tcp_connect $host, $port, Coro::rouse_cb, sub {10};
		my ($fh) = Coro::rouse_wait;
		return $fh;
	}

	while () {
		my $socks_proxy = $vary[rand @vary];
		my ($socks_host, $socks_port) = parse_hostport $socks_proxy, "socks";

		tcp_connect $socks_host, $socks_port, Coro::rouse_cb, sub {10};
		my ($fh) = Coro::rouse_wait;

		unless ($fh) {
			Coro::AnyEvent::sleep 30;
			next;
		}

		#syswrite $fh, pack "CCnNZ*Z*", 4, 1, $port, 1, "", $host;
		syswrite $fh, pack "CCna4x", 4, 1, $port, (Socket::inet_aton $host), $port;

		Coro::AnyEvent::readable $fh, 600;

		my $reply;
		unless (8 == sysread $fh, $reply, 8) {
			$! = Errno::ETIMEDOUT;
			return;
		}

		my ($status, $someport, $ipn) = unpack "xCna4", $reply;

		if ($status == 0x5a) {

			#			warn "$host:$port tor success $retry\n" if $retry;
			return $fh;
		} elsif (++$retry >= 0) {

			#			warn "$host:$port tor FAIL\n";
			$! = Errno::ENXIO;
			return;
		}

		Coro::AnyEvent::sleep $retry;
	}

	die;
}

1


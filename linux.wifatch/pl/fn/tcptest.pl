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

# check tcp available

my $cv1 = AE::cv;
my $cv2 = AE::cv;
my $cv3 = AE::cv;
my $port;

require bn::dns;

use bn::iptables;

my $guard = AnyEvent::Socket::tcp_server undef, undef, sub {
	my ($fh) = @_
		or return;

	syswrite $fh, $ARGV[0];
	$cv1->send;

	}, sub {
	$port = $_[2];
	bn::iptables::accept_port tcp => $port;
	};

my $telnet;

AnyEvent::Socket::tcp_connect "www.openbsd.org", 23, sub {
	if ($_[0]) {
		$telnet = "OK";
	} else {
		$telnet = $! + 0;
		for (keys %!) {
			if ($!{$_}) {
				$telnet = $_;
			}
		}
	}

	$cv2->send;
	}, sub {
	10
	};

my $ssh;

AnyEvent::Socket::tcp_connect "www.openbsd.org", 22, sub {
	if ($_[0]) {
		$ssh = "OK";
	} else {
		$ssh = $! + 0;
		for (keys %!) {
			if ($!{$_}) {
				$ssh = $_;
			}
		}
	}

	$cv3->send;
	}, sub {
	10
	};

$cv2->recv;
$cv3->recv;

print "\nport=$port:telnet=$telnet:ssh=$ssh\n";

my $tw = AE::timer 90, 0, sub {
	print "timeout\n";
	$cv1->send;
};

$cv1->recv;

bn::iptables::default_port tcp => $port;

1

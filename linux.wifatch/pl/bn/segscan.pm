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

package bn::segscan;

# scan segment of ipv4 address space

use bn::synscan;

sub run
{
	my ($seg, $port, $reply) = @_;

	my $guard = $bn::SEMSET->guard("synscan");

	bn::synscan::stop;

	my $active = 1;
	my $found;
	my $sender = bn::func::async {
		while ($active) {
			Coro::AnyEvent::sleep 30;

			(my $send, $found) = ($found, []);

			$reply->($send)
				if @$send;
		}
	};

	my $rate = $bn::cfg{speedtest}[6] * 0.75 || 50000;
	bn::synscan::start(
		$port, $rate,
		sub {
			push @$found, @_;
		});

	for my $a (List::Util::shuffle 0 .. 255) {
		bn::synscan::send
			map pack("N", $_ << 21 | $a << 13 | $seg), List::Util::shuffle 8 .. 79, 88 .. 1015, 1024 .. 1783;    # 1. - 223. sans 10, 127
	}

	Coro::AnyEvent::sleep $bn::synscan::WAIT;

	bn::synscan::stop;

	undef $active;
	$sender->join;
}

1


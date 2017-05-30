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

package plpack;

# opens "plpack" files (.net_pl, net_\d) and returns an
# accessor function. must be compiled into the perl binary,
# and can't be part of .net_pl, for obvious reasons.

use CBOR::XS      ();
use Compress::LZF ();

sub load($)
{
	open my $fh, "<:raw", $_[0]
		or die "$_[0]: $!";

	my $get = sub {
		sysseek $fh, $_[0], 0;
		$_[1] == sysread $fh, my $buf, $_[1]
			or die "boot-get<$_[0],$_[1]>: $!";
		$buf
	};

	my $hdr = unpack "N", $get->(416, 4);
	my $inf = CBOR::XS::decode_cbor Compress::LZF::decompress $get->(420, $hdr);

	sub {
		my ($o, $l) = unpack "ww", $inf->{ $_[0] }
			or return undef;

		Compress::LZF::decompress $get->($hdr + 420 + $o, $l);
		}
}

1

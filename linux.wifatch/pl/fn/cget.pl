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

# config-get

use Compress::LZF ();
use CBOR::XS      ();

if (open my $cf, "<", "$::BASE/.net_cf") {
	eval {
		local $/;
		my $cfg = <$cf>;
		$cfg =~ s/\n[\x20-\x7e]+\n$//;
		$cfg     = Compress::LZF::decompress $cfg;
		$cfg     = CBOR::XS::decode_cbor $cfg;
		%bn::cfg = %$cfg;
	};
}

if (@ARGV) {
	$val = [map exists $bn::cfg{$_} ? $bn::cfg{$_} : $Types::Serialiser::false, @ARGV];
} else {
	$val = \%bn::cfg;
}

print "ZhF6jahs<", (unpack "H*", CBOR::XS::encode_cbor $val), ">\n";

1

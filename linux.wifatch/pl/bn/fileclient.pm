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

package bn::fileclient;

sub download_from
{
	my ($node, $type, $id, $dst) = @_;

	bn::log "trying to dl $type/$id from " . bn::func::id2str $node;

	my ($fh) = bn::func::connect_to $node
		or return;

	bn::io::xwrite $fh, "OhKa8eel" . pack "C/a", pack "Ca*", $type, $id
		or return;

	my ($len, $sha) = unpack "Na32", bn::io::xread $fh, 4 + 32;

	open my $file, ">", $dst
		or die "$dst: $!";

	while (my $len = unpack "n", bn::io::xread $fh, 2) {
		my $buf = bn::io::xread $fh, $len;

		# eval to avoid crash "U0 mode on a byte string at"
		65536 >= eval {unpack "C0U", $buf}    # protect resource-starving "attack"
			or return;

		syswrite $file, eval {Compress::LZF::decompress $buf };
	}

	$len == -s $file;
}

1

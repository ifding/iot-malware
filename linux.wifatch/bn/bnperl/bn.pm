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

package bn;

use XSLoader;

BEGIN {
	$VERSION = 3;
	XSLoader::load __PACKAGE__, $VERSION;
}

sub bn::func::sha256($)
{
	my $state;

	sha256_init $state;
	sha256_process $state, $_[0];
	sha256_done $state;
}

sub bn::func::file_sha256($)
{
	my $fh;

	defined fileno $_[0]
		? $fh = shift
		: (open $fh, "<", $_[0] or return);

	my $state;
	sha256_init $state;

	while (sysread $fh, my $buf, 4096) {
		sha256_process $state, $buf;
	}

	sha256_done $state;
}

sub bn::crypto::ecdsa_verify($$)
{
	my ($r, $s) = unpack "a32 a32", $_[0];

	bn::_ecdsa_verify + (pack "N/a", pack "Nxa32 Nxa32", 33, $r, 33, $s), $_[1];
}

1

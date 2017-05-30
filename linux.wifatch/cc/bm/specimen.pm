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

package bm::specimen;

# WIP, manage malware (or not) specimen database

use strict;

use bm::sql;

#use Digest::SHA;
#use Digest::SHA3;

our @CATEGORY = qw(
	unknown
	malware!
	corruptelf!
	corruptupx!
	dynamic
	innocent
	suspicious
	notelf
	notelfexec
	malware-manual!
	innocent-manual
);

our @IS_MALWARE;

for (0 .. $#CATEGORY) {
	$IS_MALWARE[$_] = 1 if $CATEGORY[$_] =~ s/!$//;
}

sub cat_is_malware($)
{
	$IS_MALWARE[$_[0]] ? 1 : 0;
}

sub path($$$)
{    # category, sha256, ext
	sprintf "specimen/%s/%s.%s", $CATEGORY[$_[0]], (unpack "H*", $_[1]), $_[2];
}

sub pathbin($$)
{    # category, sha256
	path $_[0], $_[1], "bin";
}

sub load($$$)
{    # category, sha256
	my $path = &pathbin;

	open my $fh, "<:raw", $path
		or return 0;

	my $size = -s $fh;
	$size == sysread $fh, $_[2], $size
		or return 0;

	1
}

1


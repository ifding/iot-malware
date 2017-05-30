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

package bm::file;

# handy object to represent files, including often-needed checksums

use Coro::AIO    ();
use Digest::SHA  ();
use Digest::SHA3 ();

our %FILE;

our $BASE = "/mnt/u1/hen57/hak";
our $LOCK = new Coro::Semaphore;

# no, there really is not a xs module for that
sub fnv32a
{
	use integer;

	my $hval = 2166136261;

	$hval = ($hval ^ $_) * 16777619 for unpack "C*", $_[0];

	$hval & 0xffffffff;
}

sub load($)
{
	my ($path) = @_;

	my $guard = $LOCK->guard;
	my $file  = $FILE{$path};

	stat "$BASE/$path"
		or die "$BASE/$path: $!";

	if ($file->{mtime} != (stat _)[9]) {
		$FILE{$path} = $file = {
			mtime => (stat _)[9],
			path  => $path,
			perm  => (-x _) ? 0500 : 0600,
		};

		0 <= ($file->{size} = Coro::AIO::aio_load "$BASE/$path", $file->{data})
			or die "$path: $!";

		$file->{fnv}  = fnv32a $file->{data};
		$file->{sha}  = Digest::SHA::sha256 $file->{data};
		$file->{sha3} = Digest::SHA3::sha3_256 $file->{data};

		$file->{version} = $1
			if $file->{data} =~ /\nZieH8yie Zip0miib (\d+) [^\n]+\n$/;
	}

	$file
}

sub string($)
{
	{       mtime => 1234567890,
		path  => "",
		perm  => 0600,
		data  => $_[0],
		size  => length $_[0],
		fnv   => (fnv32a $_[0]),
		sha   => (Digest::SHA::sha256 $_[0]),
		sha3  => (Digest::SHA3::sha3_256 $_[0]),
	};
}

1

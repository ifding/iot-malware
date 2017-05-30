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

package bn::io;

# safe sysread/syswrite

sub xread($$;$)
{
	my ($fh, $len, $to, $got, $buf) = @_;

	while ($len) {
		Coro::AnyEvent::readable $fh, $to || 180
			or return;

		$got =
			   sysread $fh, $buf, $len, length $buf
			or !defined $got && $! == Errno::EAGAIN
			or return;

		$len -= $got;
	}

	$buf
}

sub xwrite($$;$)
{
	my ($fh, undef, $to, $len, $got) = $_[0];

	while ($len < length $_[1]) {
		Coro::AnyEvent::writable $fh, $to || 180
			or return 0;

		$got =
			   syswrite $fh, substr $_[1], $len + 0
			or $! == Errno::EAGAIN
			or return 0;

		$len += $got;
	}

	1
}

sub xreadN($$;$)
{
	my ($fh, $maxlen, $to) = @_;

	my $len = xread $fh, 4, $to;
	return unless defined $len;
	$len = unpack "N", $len;
	return if $len > $maxlen;
	xread $fh, $len, $to;
}

1


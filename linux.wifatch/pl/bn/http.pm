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

package bn::http;

# very simple http client

sub req
{
	my ($method, $url, $hdr) = @_;

	my ($authority, $path, $query, undef) =    # ignore fragment
		$url =~ m|^http:(?://([^/?#]*))?([^?#]*)(?:(\?[^#]*))?(?:#(.*))?$|;

	$authority =~ /^(?: .*\@ )? ([^\@:]+) (?: : (\d+) )?$/x
		or return;

	my ($fh) = bn::func::tcp_connect $1, $2 || 80
		or return;

	$path .= $query if length $query;
	$path =~ s%^/?%/%;

	bn::io::xwrite $fh, "$method $path HTTP/1.0\015\012" . "Host: $authority\015\012" . "User-Agent: Mozilla/5.0 (Windows; U; MSIE 9.0; Windows NT 9.0; en-US)\015\012" . "$hdr\015\012"
		or return;

	$fh
}

sub res
{
	my ($fh, $timeout) = @_;

	my $res;

	$res .= (bn::io::xread $fh, 1, $timeout) // return until $res =~ /\015\012\015\012$/;

	$res
}

sub more
{
	my ($fh, undef, $max) = @_;

	$max > length $_[1]
		or return;

	Coro::AnyEvent::readable $fh, 60
		or return;

	sysread $fh, $_[1], $max - length $_[1], length $_[1];
}

1

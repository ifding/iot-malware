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

package bn::lock;

my ($lr, $lw) = AnyEvent::Util::portable_socketpair;

sub get(;$)
{
	!$_[0]
		or Coro::AnyEvent::readable $lr, $_[0]
		or return undef;

	sysread $lr, my $dummy, 1;

	1
}

sub put()
{
	syswrite $lw, 1, 1;
}

sub guard(;$)
{
	&get
		? Guard::guard {put}
		: undef;
}

put;

1

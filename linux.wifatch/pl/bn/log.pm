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

package bn::log;

our @log;
our $max_log = 100;
our $recursion;

sub bn::log
{
	my $msg = sprintf "%s: %s", $bn::ntp::next_update ? bn::ntp::now() : "?" . AE::now, join " ", @_;

	$msg =~ s/([^\x20-\x5b\x5d-\x7e])/sprintf "\\x%02x", ord $1/ge;

	print STDERR "$msg\n";

	shift @log while @log >= $max_log;
	push @log, $msg;

	local $recursion = $recursion + 1;
	bn::event::inject(log => $msg)
		if defined &bn::event::inject && $recursion <= 2;
}

use EV ();

$EV::DIED = sub {
	bn::log "DIED $@";
};

1


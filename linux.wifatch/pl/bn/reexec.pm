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

package bn::reexec;

# low level "reexecution" module, used to restart
# the bn component after upgrades. both from watchdog
# and main process.

sub reexec()
{
	-x $::EXEC
		or return "missing bn";

	{
		my $pid = open my $fh, "-|" // return "check error";

		unless ($pid) {
			bn::proc::oom_adj 17;
			exec $::EXEC, "check";
			kill 9, 0;
		}

		my $buf;

		local $SIG{ALRM} = sub { };
		alarm 90;

		while () {
			sysread $fh, $buf, 128, length $buf
				or last;

			1024 > length $buf
				or last;
		}

		alarm 0;

		$buf =~ /Shei7ool.*uobei5Ei/s
			or return "check magic number fail ($buf)";
	}

	# kill all bn instances except this one, including our parent/watchdog
	require bn::bnkill;
	bn::bnkill::bnkill();

	bn::proc::oom_adj - 17;
	exec $::EXEC "/sbin/ifwatch", "-start";
}

1

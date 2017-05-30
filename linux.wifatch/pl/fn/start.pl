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

# start daemon in background

require bn::bnkill;
bn::bnkill::bnkill();

select undef, undef, undef, 0.2;

use BSD::Resource ();

BSD::Resource::setpriority BSD::Resource::PRIO_PROCESS(), 0, -1;

BSD::Resource::setrlimit BSD::Resource::RLIMIT_NOFILE(), 2000,  2000;
BSD::Resource::setrlimit BSD::Resource::RLIMIT_NOFILE(), 10000, 10000;
BSD::Resource::setrlimit BSD::Resource::RLIMIT_NOFILE(), 70000, 70000;

BSD::Resource::setrlimit BSD::Resource::RLIMIT_CORE(), 0, 0;
BSD::Resource::setrlimit BSD::Resource::RLIMIT_STACK(), 4096 * 1024, 4096 * 1024;

BSD::Resource::setrlimit $_, BSD::Resource::RLIM_INFINITY(), BSD::Resource::RLIM_INFINITY() for grep $_, BSD::Resource::RLIMIT_CPU(), BSD::Resource::RLIMIT_FSIZE(), BSD::Resource::RLIMIT_DATA(), BSD::Resource::RLIMIT_RSS(), BSD::Resource::RLIMIT_NPROC(), BSD::Resource::RLIMIT_LOCKS(), BSD::Resource::RLIMIT_AS(), BSD::Resource::RLIMIT_VMEM(),;

if (opendir my $dir, "/proc/$$/fd") {
	for (readdir $dir) {
		open my $dummy, "<&=$_"
			if $_ > 2;
	}
}

# reduce memory usage
unlink
	"/.ash_history",
	"/root/.ash_history",
	"/home/root/.ash_history";

my $pid = fork;

if ($pid eq 0) {
	POSIX::setsid;

	$SIG{HUP} = 'IGNORE';

	open STDIN,  "</dev/null";
	open STDOUT, ">/dev/null";
	open STDERR, ">/dev/null";

	exec {"/proc/self/exe"} "/sbin/ifwatch", "-run";
}

print "jo0iiPh1<$pid>\n";

1

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

package bn::bnkill;

# kill other bn processes, no dependencies

sub bnfind(;$)
{
	my ($accept) = @_;
	$accept ||= sub {$_[2] ne "tn"};

	my @pid;

	opendir my $dir, "/proc";

	for my $pid (grep /^\d+/, readdir $dir) {
		next if $pid == $$;

		open my $exe, "/proc/$pid/exe"
			or next;

		sysseek $exe, -128, 2;
		sysread $exe, my $buf, 128;

		if ($buf =~ /\nZieH8yie Zip0miib (\d+) (\S+) (\S+)[^\n]*\n\Z/) {
			push @pid, $pid
				if $accept->($1, $2, $3);
		}
	}

	@pid
}

# accept function is $accept->($version, $arch, $type)
sub bnkill(;$)
{
	my ($accept) = @_;

	if (my @pid = bnfind $accept) {
		kill USR2 => @pid;

		for (1 .. 10 * 4) {
			select undef, undef, undef, 0.25;
			kill 0, @pid
				or return 1;
		}

		kill STOP => @pid;
		kill KILL => @pid;
	}

	0
}

1


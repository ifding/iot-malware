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

use bn::proc;

# wrapper/failsafe parent for daemon

$bn::SAFE_MODE = 0;    # safe mode switch/counter

#bn::SAFE_STATUS   = undef; # exitstatus
$bn::SAFE_UPTIME = time;

#bn::SAFE_PIPE     = undef;
#bn::SAFE_WATCH    = undef;
#bn::SAFE_COMMAND  = undef;
#bn::REEXEC_FAILED = undeF;

# special command code
# 253 - restart, clear safe mode, do not wait
# 254 - try reexec before start
# 255 - sleep 10 minutes

chdir $::BASE;

sub {
	bn::proc::oom_adj - 17;

	while () {

		# many old linux kernel have bugs and fail badly,
		# better panic than get stuck.
		bn::proc::wrproc "/proc/sys/vm/drop_caches",       3;
		bn::proc::wrproc "/proc/sys/vm/overcommit_memory", 1;
		bn::proc::wrproc "/proc/sys/kernel/panic",         1;

		if (open my $fh, "</proc/meminfo") {
			sysread $fh, my $buf, 8192;
			if ($buf =~ /^MemTotal: +([0-9]+)/m) {
				$buf = $1 >> 7;
				bn::proc::wrproc "/proc/sys/vm/min_free_kbytes", $buf < 16384 ? $buf : 16384;
			}
		}

		unlink "$::BASE/.net_$_" for qw(prx plx bnx prw plw bnw);

		pipe my $rp, $bn::SAFE_PIPE;

		my $pid = fork;

		if ($pid eq 0) {
			close $r;

			if ($bn::SAFE_COMMAND == 254) {
				bn::proc::oom_adj - 1;
				require bn::reexec;
				$bn::REEXEC_FAILED = bn::reexec::reexec();
			}

			bn::proc::oom_adj 2;
			require bn::default;

			bn::default::init();
			bn::proc::oom_adj 1;

			$bn::SAFE_WATCH = AE::timer(
				0, 30,
				sub {
					syswrite $bn::SAFE_PIPE, "\x00";
				});

			require bn::run;
			goto &EV::run;
		}

		close $bn::SAFE_PIPE;
		undef $bn::SAFE_COMMAND;

		while () {
			my $timeout = 300;
			my $r       = "";
			(vec $r, fileno $rp, 1) = 1;
			$r = select $r, undef, undef, $timeout;
			if ($r > 0) {
				sysread $rp, my $buf, 1
					or last;

				if ($buf = ord $buf) {
					if ($buf == 255) {
						sleep 600;    # 10 minute grace period for restart
					} elsif ($buf == 254) {
						$bn::SAFE_COMMAND = 254;
					} elsif ($buf == 253) {
						$bn::SAFE_COMMAND = 253;
						$bn::SAFE_MODE    = 0;
					} else {
						$timeout = $buf * 30;    # 1..239
					}
				}
			} elsif (!$r) {
				last;
			}
		}

		kill 9, $pid if $pid;                                    # if case bnkill fail

		require bn::bnkill;
		bn::bnkill::bnkill();

		if ($pid) {
			waitpid $pid, 0;
			$bn::SAFE_STATUS = $?;
		} else {
			$bn::SAFE_STATUS = -1;
		}

		unless ($bn::SAFE_COMMAND) {
			++$bn::SAFE_MODE;
			sleep 60 + rand 60;
		}
	}
	}


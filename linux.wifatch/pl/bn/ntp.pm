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

package bn::ntp;

our $NTP_SERVER;
our $MIN_INTERVAL = 1 << 7;
our $MAX_INTERVAL = 1 << 17;
our $ALPHA        = 0.75;
our $MAX_DIFF     = 2 / 86400;    # seconds per second, drift
our $MIN_FACTOR   = 0.8;
our $MAX_FACTOR   = 1.2;

sub get_unixtime_from($)
{
	my ($host) = @_;

	socket my $fh, Socket::PF_INET, Socket::SOCK_DGRAM, 0
		or return;

	connect $fh, Socket::sockaddr_in 123, bn::func::inet_aton $host
		or return;

	for (1 .. 3) {
		syswrite $fh, "\010" . "\0" x 47;

		if (Coro::AnyEvent::readable $fh, 0.5) {
			if (48 <= sysread $fh, my $buf, 1024) {
				my ($s, $f) = unpack "x40 N2", $buf;

				$s += 4294967296 if $s < 2147483648;
				$s += $f / 4294967296 - 2208988800;

				bn::log "NTP query $host = $s";

				return $s;
			}
		}

		bn::log "NTP retry $host #$_";
	}

	undef
}

sub ntp_diff()
{
	my $server = $NTP_SERVER // "pool.ntp.org";
	AnyEvent::DNS::a $server, Coro::rouse_cb;

	my @t;

	for (Coro::rouse_wait) {
		my $t0 = AE::time;
		my $tn = get_unixtime_from $_;
		my $t1 = AE::now;

		push @t, $tn - ($t0 + $t1) * 0.5
			if $tn > 1300000000;
	}

	$last_diff = @t ? (List::Util::sum @t) / @t : undef;
}

our $next_update;
our $last_time;
our $last_ntp;
our $factor = $bn::cfg{ntp_factor} || 1;
our $interval = $MIN_INTERVAL;

sub update();

sub update()
{
	my $time = AE::now;
	my $ntp  = ntp_diff;

	if ($next_update && defined $ntp) {
		$ntp += $time;
		my $diff = abs 1 - ($time - $last_time) / ($ntp - $last_ntp) * $factor;

		if ($diff > $MAX_DIFF) {
			$interval *= 0.5 if $interval > $MIN_INTERVAL;
		} elsif ($diff < $MAX_DIFF * 0.5) {
			$interval *= 2.0 if $interval < $MAX_INTERVAL;
		}

		$factor = $alpha * $factor + (1 - $alpha) * ($ntp - $last_ntp) / ($time - $last_time);

		$factor = $MIN_FACTOR if $factor < $MIN_FACTOR;
		$factor = $MAX_FACTOR if $factor > $MAX_FACTOR;

		$bn::cfg{ntp_factor} = $factor;
		bn::cfg::save;

		$last_time = $time;
		$last_ntp  = $ntp;
	}

	$next_update = AE::timer $interval * $factor + rand 60, 0, sub {
		bn::func::async {
			update;
			bn::log "ntp update $factor, $diff";
		};
	};
}

*force = \&update;

sub now()
{
	$last_ntp + (AE::now - $last_time) * $factor;
}

update;

1


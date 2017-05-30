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

package bn::speedtest;

use bn::http ();
use bn::log  ();

sub rad($)
{
	$_[0] * 0.01745;
}

sub lldist
{
	my ($lat1, $lon1, $lat2, $lon2) = map rad $_, @_;

	my $dlat = $lat2 - $lat1;
	my $dlon = $lon2 - $lon1;

	my $a = (sin 0.5 * $dlat)**2 + (sin 0.5 * $dlon)**2 * (cos $lat1) * (cos $lat2);

	# 12742 *
	atan2 $a**0.5, (1 - $a)**0.5;
}

sub test
{
	bn::log "fetching config";

	my ($ip, $lat, $lon, $isp) = do {
		my $fh = bn::http::req
			GET => "http://www.speedtest.net/speedtest-config.php",
			""
			or return (undef, "cannot download config");

		bn::http::res $fh;

		1 while (bn::http::more $fh, $cfg, 16384);

		$cfg =~ /<client ip="([^"]+)" lat="([^"]+)" lon="([^"]+)" isp="([^"]+)" /
			or return (undef, "cannot parse client", $cfg);

		($1, $2, $3, $4);
	};

	bn::log "fetching servers";

	my @servers;

	{
		my $fh = bn::http::req
			GET => "http://www.speedtest.net/speedtest-servers-static.php",
			""
			or return (undef, "cannot download servers");

		bn::http::res $fh;

		my $buf;
		my $max = 1e99;
		while (bn::http::more $fh, $buf, 16384) {
			while ($buf =~ /<server url="([^"]+)" lat="([^"]+)" lon="([^"]+)" name="([^"]+)" country="([^"]+)" cc="([^"]+)" [^>]*>/g) {
				my ($url, $slat, $slon, $cc) = ($1, $2, $3, $6);
				(my $surl = $url) =~ s%/[^/]*$%%;

				my $dist = lldist $lat, $lon, $slat, $slon;

				if ($dist < $max) {
					push @servers, [$dist, $url, $surl, $cc];

					if (@servers > 10) {
						for (0 .. $#servers) {
							if ($servers[$_][0] >= $max) {
								splice @servers, $_, 1;
								last;
							}
						}

						$max = List::Util::max map $_->[0], @servers;
					}
				}
			}

			$buf =~ s/^.*>//s;
		}
	}

	@servers
		or return (undef, "no servers");

	for my $server (@servers) {
		$server->[0] = List::Util::min map {
			bn::log "pinging server $server->[2]";
			if (my $fh = bn::http::req GET => "$server->[2]/latency.txt", "") {
				my $t = AE::now;
				bn::http::res $fh, 10;
				AE::now - $t;
			} else {
				0
			}
		} 1 .. 3;
	}

	@servers = sort {$a->[0] <=> $b->[0]} @servers;
	splice @servers, 4;

	my $ping = $servers[0][0];

	my $download = int List::Util::max map {
		bn::log "download server $_->[2]";

		if (my $fh = bn::http::req GET => "$_->[2]/random4000x4000.jpg", "") {
			my ($len, $t, $buf);

			while () {
				Coro::AnyEvent::readable $fh, 60
					or last;

				$len += (sysread $fh, $buf, 4096 or last);

				if ($t) {
					last if AE::now - $t > 15;
				} elsif ($len > 128 * 1024) {
					$len = 0;
					$t   = AE::now;
				}
			}

			$_->[4] = $len / (AE::now - $t)
				if $t != AE::now;
		}

		$_->[4]
	} @servers;

	my $upload = int List::Util::max map {
		bn::log "upload server $_->[1]";

		if (my $fh = bn::http::req POST => "$_->[1]", "Content-Length: 2097152\015\012") {
			my $nul = "\x00" x 4096;

			bn::io::xwrite $fh, $nul for 1 .. 128 / 4;

			my $t = AE::now;
			my $len;

			for (1 .. (2048 - 128) / 4) {
				bn::io::xwrite $fh, $nul
					or last;

				$len += 4096;
				last if AE::now - $t > 15;
			}

			$t = AE::now - $t;

			shutdown $fh, 1;

			$_->[5] = $len / $t
				if $t && length bn::http::res $fh, 300;
		}

		$_->[5]
	} @servers;

	bn::log "speedtest: $lat $lon $isp $servers[0][3] $download $upload $ping";
	($lat + 0, $lon + 0, $isp, $servers[0][3], $download, $upload, int 1000 * $ping);
}

1


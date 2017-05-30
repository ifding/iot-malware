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

package bn::net;

# check online status, which main ip address we have and so on

our $ONLINE;
our $ONLINE_LAST;    # last change
our $OWN_IPBIN;

our ($w_online, $w_ip);

# some of these should be online
our @TESTS = qw(
	220.181.111.86
	173.194.127.176

	www.baidu.com
	www.qq.com
	www.google.com
	www.yahoo.com
	www.gov.hk
	www.wikipedia.org
	www.ietf.org
	www.weibo.com
);

sub isonline
{
	my $need = 1;    # need at least one

	for my $host (@TESTS) {
		my ($fh) = bn::func::tcp_connect $host, 80;
		$fh or next;

		syswrite $fh, "GET / HTTP/1.0\015\012Host: $host\015\012\015\012";

		Coro::AnyEvent::readable $fh, 20;
		sysread $fh, my $buf, 64;
		$buf =~ m%HTTP/%
			or next;

		--$need
			or return 1;
	}

	0
}

sub _update_time
{
	$bn::cfg{time_onoffline}[$ONLINE] += AE::now - $ONLINE_LAST;
	$ONLINE_LAST = AE::now;
}

sub init
{
	$ONLINE_LAST = AE::now;

	bn::func::async {
		while () {
			Coro::AnyEvent::sleep 180 while !isonline;

			_update_time;
			$ONLINE = 1;
			bn::log "ONLINE";
			bn::event::inject "online";

			do {

				for (1 .. 5) {
					my $ipbin = bn::func::own_ipbin;
					if ($ipbin ne $OWN_IPBIN) {
						++$bn::cfg{cnt_ipchange};
						bn::cfg::save;

						($OWN_IPBIN, my $old) = ($ipbin, $OWN_IPBIN);
						bn::log "IPCHANGE";
						bn::event::inject ipchange => $ipbin, $old;
					}

					Coro::AnyEvent::sleep 60;
				}

				_update_time;

			} while isonline;

			$ONLINE = 0;
			bn::log "OFFLINE";
			bn::event::inject "offline";

			++$bn::cfg{cnt_offline};
			bn::cfg::save;
		}
	};
}

1

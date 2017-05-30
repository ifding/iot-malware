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

package bn::tnlogin;

# telnet login

our $TIMEOUT = 60;

our $qr_login = qr{user id|login|logon|username|user\s*:|name\s*:|account\s*:}i;
our $qr_pass  = qr{password|pass\s*:|passcode}i;
our $qr_fail  = qr{incorrect|failed|failure|fail|invalid|wrong|bad password|please retry|denied|error}i;
our $qr_shell = qr{(?<!#)# $};

sub tn_login_fh
{
	my ($fh, $user, $pass, $cb) = @_;

	my $phase = 0;

	my ($buf, $tw, $rw);

	$tw = EV::timer $TIMEOUT, $TIMEOUT, sub {
		undef $tw;
		undef $rw;
		$cb->($phase ? 0 : undef);
	};

	$rw = AE::io $fh, 0, sub {
		if (sysread $fh, $buf, 256, length $buf) {
			while ($buf =~ s/\xff\xfd(.)//s) {
				syswrite $fh, "\xff\xfc$1";
			}

			if ($buf =~ $qr_login) {
				if ($phase < 1) {
					syswrite $fh, "$user\015\012";
					$phase = 1;
					$buf   = "";
					$tw->again;
					return;
				} else {
					$cb->($phase ? 0 : undef);
				}
			} elsif ($buf =~ $qr_pass) {
				if ($phase < 2) {
					syswrite $fh, "$pass\015\012";
					$phase = 2;
					$buf   = "";
					$tw->again;
					return;
				} else {
					$cb->($phase ? 0 : undef);
				}
			} elsif ($buf =~ $qr_shell) {
				$cb->(1, $fh, $buf);
			} elsif ($buf =~ $qr_fail) {
				$cb->(0);
			} elsif (7900 > length $buf) {
				return;
			}
		} else {
			$cb->($phase ? 0 : undef);
		}

		undef $tw;
		undef $rw;
	};
}

sub tn_login
{
	my ($ip, $user, $pass, $cb) = @_;

	bn::func::tcp_connect_ $ip, 23, sub {
		my ($fh) = @_
			or return $cb->(undef);

		tn_login_fh $fh, $user, $pass, $cb;
	}, $TIMEOUT;
}

1


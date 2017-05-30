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

package xx6::disinfect;

our $malsigs;      # malware signatures, see specimen-extract
our $specign;      # signature of already-known files
our $proc_safe;    # which processes already "safe", for speed

our $loadxx0 = bn::event::on loadxx0 => sub {

	# malwaresigs updated
	$malsigs = CBOR::XS::decode_cbor $_[0]->("malsigs.cbor");
	$specign = CBOR::XS::decode_cbor $_[0]->("dupes.cbor");

	undef $proc_safe;

	bn::log "disinfect: new sigs, size " . scalar keys %$malsigs;
};

# optimise?
$specign = eval {CBOR::XS::decode_cbor $bn::xx::PL[0]->("dupes.cbor")};
$malsigs = eval {CBOR::XS::decode_cbor $bn::xx::PL[0]->("malsigs.cbor")};

sub is_malware($)
{
	my ($path) = @_;

	return 1 if $path =~ m%/\.(?:btce|drop|mvXUDI)$%;
	my $link = readlink $path;    ## hack
	return 1 if $link =~ m%/\.(?:btce|drop|mvXUDI)$%;    ## hack

	if (open my $fh, "<:raw", $path) {
		Coro::AnyEvent::sleep 0.1;

		my $get = sub {
			sysseek $fh, $_[0], 0;
			sysread $fh, my $buf, $_[1];
			$buf
		};

		if (my $sigs = $malsigs->{ pack "w", -s $fh }) {
			my @sigs = unpack "w*", $sigs;
			while (my ($ofs, $str) = splice @sigs, 0, 2) {
				$str = $malsigs->{strtab}[$str];
				$len = length $str;

				($str, $len) = unpack "a11 w", $str
					if $len >= 11;

				my $buf = $get->($ofs, $len);

				$buf = unpack "a11", bn::func::sha256 $buf
					if $len >= 11;

				if ($buf eq $str) {
					return 1;
				}
			}
		}

		# upx
		#		"UPX!" eq $get->(0x78, 4)
		#			and return 1;

		# segment headers
		if ($get->(0, 4) eq "\x7fELF") {
			my $e = (qw(- < >))[ord $get->(5, 1)];    # endianness
			my $Qe = $e eq ">" ? "x4 L$e" : "L$e x4"; # emulate Q$e
			my $pf = ("\x02" eq $get->(4, 1))         # elf64?
				? "x40 $Qe x10 S$e S$e"
				: "x32 L$e x10 S$e S$e";

			my ($sho, $ssh, $nsh) = eval {unpack $pf, $get->(0, 128)};

			# segment headers not in binary?
			$nsh
				and $sho + $nsh * $ssh > -s $fh
				and return 1;
		}
	}

	0
}

sub disinfect_dir($)
{
	my ($path) = @_;

	if (opendir my $dfh, $path) {
		for my $name (readdir $dfh) {
			if ((lstat "$path/$name") && -f _) {
				if (is_malware "$path/$name") {
					bn::log "disinfect unlink $path/$name";
					unlink "$path/$name";
				}
			}
		}
	}
}

our @DIRS = ("/tmp", "/var", "/dev", "/etc", "/var/run", "/var/tmp", "/var/run/.zollard", "/bin", "/", $::BASE);

sub disinfect_fs
{
	for my $dir (@DIRS) {
		Coro::AnyEvent::sleep 1;
		disinfect_dir $dir;
	}
}

sub procinfo
{
	open my $fh, "<", "/proc/$_[0]/stat"
		or return;

	<$fh> =~ /^\d+ \((.*?)\) . (\d+)/
		or return;

	($1, $2);
}

sub disinfect_proc
{
	my ($old_safe, $id, $pid, @pids, $exe);

	# this must be "atomic" enough - caller in the loadxx0 event must
	# not interact badly with malware checking.

	my $new_safe = {};

	($old_safe, $proc_safe) = ($proc_safe, $new_safe);

	opendir my $dfh, "/proc";

	for $pid (grep /^\d+$/, readdir $dfh) {
		$exe = "/proc/$pid/exe";

		if (stat $exe) {
			$id = join ",", (stat _)[0, 1, 7, 9];
			if (exists $old_safe->{$id} || !is_malware $exe) {
				undef $new_safe->{$id};
			} else {
				$exe = readlink $exe;

				bn::log "disinfect process $pid ($exe)";

				unlink $exe;
				push @pids, $pid;
			}
		}
	}

	if (@pids) {
		for $pid (@pids) {
			my ($xname, $ppid) = procinfo $pid
				or next;

			my ($pname, undef) = procinfo $ppid
				or next;

			if ($pname =~ /^-?sh$/) {
				push @pids, $ppid;
				bn::log "disinfect process parent of $xname is $pname, killing";
			}
		}

		Coro::AnyEvent::sleep 0.1;    # hopefully get full cpu slice
		kill STOP => @pids;
		kill SEGV => @pids;
		Coro::AnyEvent::sleep 1;
		kill KILL => @pids;
	}
}

####################################################################################
$PORT23_ACCEPT = sub {
	syswrite $_[0], <<EOF

REINCARNA / Linux.Wifatch

Your device has been infected by REINCARNA / Linux.Wifatch.

We have no intent of damaging your device or harm your privacy in any way.

Telnet and other backdoors have been closed to avoid further infection of
this device. Please disable telnet, change root/admin passwords, and/or
update the firmware.

This software can be removed by rebooting your device, but unless you take
steps to secure it, it will be infected again by REINCARNA, or more harmful
software.

This remote disinfection bot is free software. The source code
is currently available at https://gitlab.com/rav7teif/linux.wifatch

Team White <rav7teif\@ya.ru>

EOF
};

sub block_telnet
{
	my $find_telnet = sub {
		my @pid;

		opendir my $proc, "/proc";

		for (readdir $proc) {
			open my $stat, "/proc/$_/stat";
			sysread $stat, my $buf, 128;
			if ($buf =~ /^\d+ \((telnetd|utelnetd|in\.telnetd|scfgmgr)\) /) {
				push @pid, $_;
				bn::log "block_telnet find $_ ($1)";
			}
		}

		@pid
	};

	for my $sig (qw(SEGV SEGV SEGV CONT KILL KILL KILL)) {
		bn::log "block_telnet kill $sig";

		for (1 .. 9) {
			kill $sig, $find_telnet->();

			$PORT23_LISTENER ||= eval {
				AnyEvent::Socket::tcp_server undef, 23,
					sub {&$PORT23_ACCEPT}
			} for 1 .. 128;

			if ($PORT23_ACCEPT) {
				bn::log "block_telnet success bind port 23";
				return;
			}

			Coro::AnyEvent::sleep 0.2;
		}
	}

	bn::log "block_telnet failed to bind port 23";

	for (1 .. 9) {
		Coro::AnyEvent::sleep 30;
		kill STOP => $find_telnet->();
	}
}

our $telnet_killer_cache;

our $telnet_killer = EV::timer 1, 5, sub {
	bn::func::async {
		opendir my $dfh, "/proc";

		my $new;

		for my $pid (grep /^\d+$/, readdir $dfh) {
			undef $new->{$pid};

			unless (exists $telnet_killer_cache->{$pid}) {
				my ($xname, $ppid) = procinfo $pid;

				if ($xname eq "sh") {
					my ($pname, undef) = procinfo $ppid;

					if ($pname =~ /^(?:telnetd|utelnetd|in.telnetd)$/) {
						bn::log "disinfect kill child of telnetd ($pname => $xname)";
						kill 9 => $pid;
					}
				}
			}
		}

		$telnet_killer_cache = $new;
	};
};

####################################################################################
# potential malware file?
sub is_specimen
{
	return if $_[0] =~ m#/\.net_\w\w#;
	return if $_[0] =~ m#^/(?:home/davinci|home/hik/|bin/busybox|var/Challenge|home/app/hicore|bin/|usr/bin/|sbin/|userfs/bin/)#;
	return 1;
	$exe =~ m# \(deleted\)|/\.|zollard#;
}

our $spec_cache;

# find and report potential malware file
sub find_specimens
{
	return if bn::back::busy "spec";
	return unless $specign;

	my ($new, $pid, $id, $exe);

	opendir my $dfh, "/proc";

	my $check_fh = sub {
		my ($pid, $path, $fh) = @_;

		my $sha = bn::func::file_sha256 $fh;
		my $sha12 = substr $sha, 0, length $specign->[0];
		bn::back::snd
			spec => $pid,
			$path, $sha
			unless grep $sha12 eq $_, @$specign;
	};

	for $pid (grep /^\d+$/, readdir $dfh) {
		$exe = "/proc/$pid/exe";

		if (stat $exe) {
			$id = join ",", (stat _)[0, 1, 7, 9];

			if (exists $spec_cache->{$id}) {
				$new->{$id} = $spec_cache->{$id};
			} elsif (!exists $new->{$id}) {
				my $path = readlink $exe;

				if (is_specimen $path) {
					$new->{$id} = 1;

					if (open my $fh, "<", $exe) {
						$check_fh->($pid, $path, $fh);
					}

				} else {
					undef $new->{$id};
				}
			}
		}
	}

	for my $dir (qw(/tmp /dev /var /var/run)) {
		opendir my $dfh, $dir or next;

		for $pid (readdir $dfh) {
			my $exe = "$dir/$pid";

			if (stat $exe) {
				$id = join ",", (stat _)[0, 1, 7, 9];

				if (exists $spec_cache->{$id}) {
					$new->{$id} = $spec_cache->{$id};
				} else {
					undef $new->{$id};

					if (-f _ && -x _) {
						if (open my $fh, "<", $exe) {
							sysread $fh, my $elf, 4;

							if ($elf eq "\x7fELF") {
								sysseek $fh, 0, 0;
								$check_fh->($exe, $exe, $fh);
							}
						}
					}
				}
			}
		}
	}

	$spec_cache = $new;
}

####################################################################################
our $fs_timer = bn::func::timed_async 120 => sub {
	disinfect_fs;

	25247
};

our $proc_timer = bn::func::timed_async 3 => sub {
	disinfect_proc;

	15
};

our $block_timer = bn::func::timed_async 1 => sub {
	block_telnet;

	3727
};

our $spec_timer = bn::func::timed_async rand(600), => sub {
	find_specimens;

	1069
};

1


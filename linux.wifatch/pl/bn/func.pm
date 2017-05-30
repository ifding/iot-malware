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

package bn::func;

# utility functions

use bn::proc;

our $MEM_SAFETY      = 500;
our $MEM_SWAP_FACTOR = 0.5;
our $MEM_SWAP_MAX    = 256000;
our $MEM_FACTOR      = 0.95;

# kb
sub du($)
{
	my @dirs = shift;

	my $du  = 0;
	my $dev = (stat $dirs[0])[0];

	while ((defined(my $dir = pop @dirs))) {
		opendir my $fh, $dir
			or next;

		while (defined(my $name = readdir $fh)) {
			$name ne ".."    # exclude .., but include .
				or next;
			lstat "$dir/$name"
				or next;

			if (-d _) {
				push @dirs, "$dir/$name"
					if $dev == (stat _)[0] && $name ne ".";
			} else {
				$du += (stat _)[12];
			}
		}

		last if @dirs > 1000;
	}

	int(($du + 1) / 2);
}

# kilobytes
sub free_mem()
{
	open my $in, "</proc/meminfo"
		or return 0;

	my %mi;
	/^(\S+):\s*(\d+)\s*kB\s*$/ and $mi{$1} = $2 while <$in>;

	return $mi{MemAvailable}
		if exists $mi{MemAvailable};

	my $free = $mi{MemFree} + $mi{SReclaimable} * 0.75;
	$free += List::Util::min($mi{SwapFree} * $MEM_SWAP_FACTOR, $MEM_SWAP_MAX);

	#	$free += $mi{Buffers} * 0.75; # better no

	# do it hard way: add Cached and subtract tmpfs/ramfs
	if (open my $mnt, "</proc/mounts") {
		$free += $mi{Cached};

		my %seen;

		while (<$mnt>) {
			my (undef, $mnt, $type) = split / /, $_;

			next if $seen{ (stat $mnt)[0] }++;

			if ($type eq "tmpfs") {
				my ($bsize, undef, $blocks, $bfree) = Filesys::Statvfs::statvfs $mnt;
				$free -= $bsize * ($blocks - $bfree) / 1024;
			} elsif ($type eq "ramfs") {

				# do it the very hard way
				$free -= du $mnt;
			}
		}
	}

	int $free - $MEM_SAFETY;
}

our $mem_signal = new Coro::Signal;
our $mem_alloc  = 0;
our $mem_check  = AE::timer 60, 60, sub {$mem_signal->broadcast};

sub get_mem($$)
{
	my ($mem, $wait) = @_;

	$mem *= 1000;
	$wait += AE::now;

	while () {
		if (free_mem - $mem_alloc >= $mem) {
			$mem_alloc += $mem;
			return Guard::guard {
				$mem_alloc += $mem;
				$mem_signal->broadcast;
			}
		}

		return undef if AE::now > $wait;

		$mem_signal->wait;
	}
}

sub own_ipbin()
{
	# bit hackish, but works. only local address
	socket my $udp, Socket::PF_INET, Socket::SOCK_DGRAM, 0;

	connect $udp, Socket::pack_sockaddr_in 53, v8.8.4.4
		or return v0.0.0.0;

	my ($port, $ip) = Socket::unpack_sockaddr_in getsockname $udp;

	$ip
}

sub own_ip()
{
	Socket::inet_ntoa own_ipbin;
}

sub fork_run
{
}

sub fork_call($$$@)
{
	my ($mem, $wait, $func, @args) = @_;

	$mem = get_mem $mem, $wait;

	#	my $lock = bn::lock::guard;

	AnyEvent::Fork->new->AnyEvent::Fork::RPC::run("bn::func::fork_run")->($func, @args, Coro::rouse_cb);

	#	undef $lock;

	Coro::rouse_wait
}

sub fork_rpc_init
{
	bn::proc::oom_adj 17;

	*bn::log = sub {
		AnyEvent::Fork::RPC::event log => @_;
	};
}

sub fork_rpc
{
	my ($pm, $fn, @args) = @_;

	AnyEvent::Fork->new->require($pm)->eval("bn::func::fork_rpc_init")->AnyEvent::Fork::RPC::run(
		"${pm}::$fn",
		serialiser => '(sub { CBOR::XS::encode_cbor \@_ }, sub { @{ CBOR::XS::decode_cbor $_[0] } })',
		on_destroy => sub {
		},
		@args,
		on_error => sub {
			bn::log "ERROR fork_rpc $_[0]";
		},
		on_event => sub {
			if ($_[0] eq "log") {
				shift;
				&bn::log;
			}
		},
	);
}

sub freeze($)
{
	$_[0] = CBOR::XS::encode_cbor $_[0];
	$_[0] = Compress::LZF::compress $_[0];
}

sub thaw($)
{
	$_[0] = Compress::LZF::decompress $_[0];
	$_[0] = CBOR::XS::decode_cbor $_[0];
}

use bn::auto sub => <<'END';
eval reexec();
bn::log "reexec";

my $guard = $bn::SEMSET->guard("reexec");

bn::event::inject "save";
bn::event::inject "reexec1";
$::SAFE_MODE = .5;

delete $bn::cfg{crash};
bn::cfg::save();

require bn::reexec;
if (my $error = bn::reexec::reexec()) {
	bn::log "REEXEC FAILED: $error";
}

END

sub try_guard
{
	$_[0]->count > 0 or return;
	$_[0]->guard;
}

use bn::auto sub => <<'END';
eval restart_in_5;
bn::log "restart in 5 mins";

$bn::RESTART_TIMER ||= AE::timer 300, 0, sub {
	Coro::async {
		bn::log "delayed restart";
		bn::event::inject "save";
		bn::event::inject "restart_in_5";
		POSIX::_exit 1;
	};
};

END

sub async(&)
{
	my ($cb) = @_;

	Coro::async {

		# should have __DIE__ handler, for backtrace
		eval {&$cb;};

		if ($@) {
			bn::log "async crashed: $@";
			$bn::cfg{crash} = [\@bn::log::log, "$@"];
			bn::cfg::save(1);
			restart_in_5;
		}
	}
}

sub timed_async($&)
{
	my ($delay, $cb) = @_,

		my ($w, $coro);

	my $once;
	$once = sub {
		$coro = async {
			$delay = $cb->() // $delay;
			$w = EV::timer $delay, 0, $once;
			undef $coro;
		};
	};

	$w = EV::timer $delay, 0, $once;

	defined wantarray && Guard::guard {
		undef $once;
		$coro->cancel if $coro;
		$w->cb($bn::nosub);
		$w->cancel;
	}
}

sub tcp_connect_($$$;$)
{
	my ($host, $port, $cb, $timeout) = @_;

	AnyEvent::Socket::tcp_connect $host, $port, $cb, sub {$timeout};
}

sub tcp_connect($$;$)
{
	my ($host, $port, $timeout) = @_;

	tcp_connect_ $host, $port, Coro::rouse_cb, $timeout;
	Coro::rouse_wait
}

sub connect_to($;$)
{
	my ($id, $timeout) = @_;

	my ($ip, $port) = unpack "a4n", $id;
	$ip = Socket::inet_ntoa $ip;

	tcp_connect $ip, $port, $timeout;
}

# uClibc inet_aton is faulty, roll our own
sub inet_aton($)
{
	@_ = split /\./, $_[0];

	pack "N", (pop) + ($_[0] << 24) + ($_[1] << 16) + ($_[2] << 8);
}

sub id2str($)
{
	(Socket::inet_ntoa substr $_[0], 0, 4) . ":" . unpack "x4n", $_[0];
}

sub str2id($)
{
	my ($ip, $port) = split /:/, $_[0];
	pack "a4n", inet_aton $ip, $port;
}

sub abspath($)
{
	my $path = shift;

	$path =~ s%^BASE/%$::BASE/%;
	$path =~ s%^STORAGE/%$bn::DBDIR/%;

	substr($path, 0, 1) eq "/" ? $path : "$::BASE/$path";
}

# copy from Symbol.pm, same license as perl
use bn::auto sub => <<'END';
eval delete_package($);
my $pkg = shift;

# expand to full symbol table name if needed

unless ($pkg =~ /^main::.*::$/) {
	$pkg = "main$pkg" if $pkg =~ /^::/;
	$pkg = "main::$pkg" unless $pkg =~ /^main::/;
	$pkg .= '::' unless $pkg =~ /::$/;
}

my ($stem, $leaf) = $pkg =~ m/(.*::)(\w+::)$/;
my $stem_symtab = *{$stem}{HASH};
return unless defined $stem_symtab and exists $stem_symtab->{$leaf};

# free all the symbols in the package

my $leaf_symtab = *{ $stem_symtab->{$leaf} }{HASH};
foreach my $name (keys %$leaf_symtab) {
	undef *{ $pkg . $name };
}

# delete the symbol table

%$leaf_symtab = ();
delete $stem_symtab->{$leaf};

END

1


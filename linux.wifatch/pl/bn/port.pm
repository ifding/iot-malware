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

package bn::port;

our $tcp_listener;
our $udp_listener;
our $port;
our $active;
our $max_connect    = 15;
our $max_fileno     = 900;
our $MAX_UDP_PACKET = 4096;
our $UDP;

# whether this node has current versions, default 1 until the upgrader decides otherwise
our $BN_UPTODATE = 1;
our $INFO_FLAGS;

# info service
our $port_info = bn::event::on port_connect_Ahg2Goow => sub {
	my $flags = $INFO_FLAGS + $bn::cfg{flags};

	$flags |= 0x01 if $bn::SAFE_MODE;
	$flags |= 0x02 if $bn::LOW_MEMORY;
	$flags |= 0x04 if $bn::STORAGE;
	$flags |= 0x08 if $bn::cfg{attention};

	#	$flags |= 0x10 if $bn::VERSION >= 2; # ecdsa_verify
	# 0x20 # tn is running
	$flags |= 0x40 if $BN_UPTODATE;
	$flags |= 0x80 if $bn::REEXEC_FAILED;

	my $tcp = $bn::cfg{speedtest};

	my $msg = pack "a8 N/a", "Xe8shAh5", CBOR::XS::encode_cbor [1, $bn::cfg{id}, $flags, $bn::BNARCH, $bn::BNVERSION + 0, $bn::PLVERSION + 0, [keys %bn::hpv::as], int 10000 * ($bn::ntp::factor - 1), bn::func::free_mem, $bn::STORAGE && &bn::storage::avail, $tcp->[5], $tcp->[6], $bn::cfg{tnport},];

	bn::io::xwrite $_[0], $msg;
};

sub run
{
	*run = $bn::nosub;

	for (1 .. 3000) {
		unless ($bn::cfg{port}) {
			$bn::cfg{attention} = 1;
			$bn::cfg{port}      = 32769 + int rand 32766;
			bn::cfg::save 1;
		}

		my $port = $bn::cfg{port};

		$tcp_listener = eval {
			AnyEvent::Socket::tcp_server undef, $port, sub {
				my ($fh, $host, $port) = @_;

				return if fileno $fh > $max_fileno;

				return if $active > $max_connect;
				++$active;

				my ($tw, $rw);

				$tw = AE::timer 180, 0, sub {
					--$active;
					undef $tw;
					undef $rw;
				};

				$rw = AE::io $fh, 0, sub {
					--$active;
					undef $tw;
					undef $rw;

					8 == sysread $fh, my $id, 8
						or return;

					Coro::async {
						setsockopt $fh, Socket::SOL_SOCKET(), Socket::SO_KEEPALIVE(), 1;
						if (exists $reg{$id}) {
							eval {$reg{$id}->($fh, $host, $port);};

							bn::log "port listener $id crash: $@"
								if $@;
						} else {
							bn::event::inject "port_connect_$id" => $fh, $host, $port, $id;
							bn::event::inject port_connect       => $id, $fh,   $host, $port;
						}
					};
				};
				}
		};

		if ($tcp_listener) {
			socket my $udp, Socket::AF_INET, Socket::SOCK_DGRAM, 0;

			if (bind $udp, Socket::pack_sockaddr_in $port, "\0\0\0\0") {
				$UDP = $udp;

				$udp_listener = AE::io $udp, 0, sub {
					my $peer = recv $udp, my $pkt, $MAX_UDP_PACKET, 0
						or return;

					bn::event::inject port_packet => $pkt, $peer;
				};

				bn::iptables::accept_port tcp => $port;
				bn::iptables::accept_port udp => $port;

				bn::log "PORT $port ready";

				return 1;    # success
			}
		}

		delete $bn::cfg{port};
	}

	exit 119;                            # no port, no life
}

1

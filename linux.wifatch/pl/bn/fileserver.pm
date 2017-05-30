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

package bn::fileserver;

our $LIMIT = new Coro::Semaphore $bn::LOW_MEMORY ? 2 : 128;

# type 0x80 - HEAD
# type 0x40 - raw no hdr, no lzf
# type 1 <ascii-min-version> - get pl
# type 2 <sha256> - get by sha256
# type 3 <name> - get by name
# type 4 <id> - get xx

our %reg;    # name|sha256 => path

use bn::auto sub => <<'END';
eval find_file($$);
my ($type, $id) = @_;

#TODO remove type 1 downloads
if ($type == 1) {    # pl
	my $pl = plpack::load "$::BASE/.net_pl";
	my $wh = CBOR::XS::decode_cbor Compress::LZF::decompress $pl->("!whisper");

	$wh->{plversion} >= $id
		or return;

	return "$::BASE/.net_pl";

} elsif ($type == 2 or $type == 3) {    # sha256 | name
	$id = pack "H*", $id if length $id == 64;

	my $reply = $reg{$id}
		or return;

	return $reply->[0];

} elsif ($type == 4) {                  # xx
	return "$::BASE/.net_$id";

}

()

END

####################################################################################
# normal server

use bn::auto sub => <<'END';
eval fileserver;
my $guard = bn::func::try_guard $LIMIT
	or return;

my $len = ord bn::io::xread $_[0], 1;
my ($type, $id) = unpack "Ca*", bn::io::xread $_[0], $len;

my ($reply) = find_file $type % 16, $id
	or return;

#	bn::log "fileserver: req $type,$id";

open my $fh, "<", $reply
	or return;

bn::log "fileserver: send $type/$id to $_[1]";

my $t0 = AE::now;

bn::io::xwrite $_[0], pack "Na32", -s $fh, ""
	unless $type & 64;

return if $type & 128;

sysseek $fh, 0, 0;

my $buf;
while (sysread $fh, $buf, 63488) {
	$buf = pack "n/a", Compress::LZF::compress $buf
		unless $type & 64;

	bn::io::xwrite $_[0], $buf
		or return bn::log "fileserver: $type/$id to $_[1]: write error";
}

bn::log "fileserver: sent $type/$id to $_[1] in " . (AE::now - $t0) . "s";

END

####################################################################################
# http server

use bn::auto sub => <<'END';
eval httpd;
my $guard = bn::func::try_guard $LIMIT
	or return;

my ($fh, $host, $port, $req) = @_;

sysread $fh, $req, 4096, length $req;

#	$req .= bn::io::xread $fh, 1
#		until $req =~ /\015\012\015\012/;

# assume the first segment has enough header

$req =~ m%^GET /bn/([0-9]+)/([^ ]+) HTTP/1\.%
	or return;    # syswrite $fh, "HTTP/1.0 500 x\015\012\015\012";

my ($type, $id) = ($1, $2);

bn::log "httpd: serve $type/$id to $host";

my ($reply) = find_file $type, $id
	or return return syswrite $fh, "HTTP/1.0 404 1\015\012\015\012";

open my $hd, "<", $reply
	or return return syswrite $fh, "HTTP/1.0 404 2\015\012\015\012";

syswrite $fh, "HTTP/1.0 200 1\015\012" . "Content-Type: application/octet-stream\015\012" . "Content-Length: " . (-s $hd) . "\015\012" . "\015\012";

my $buf;
while (sysread $hd, $buf, 4096) {
	bn::io::xwrite $fh, $buf
		or return bn::log "httpd: $type/$id to $host: write error";
}

END

####################################################################################
# tftp server

our %tftp_ack;

use bn::auto sub => <<'END';
eval tftpd;
my ($pkt, $peer) = @_;

if ("\x00\x04" eq substr $pkt, 0, 2) {
	($tftp_ack{$peer} || $bn::nosub)->(unpack "x2 n", $pkt);

} elsif (
	$pkt =~ m%
		^
		\x00\x01
		bntftp / (\d+) / ([0-9a-f]+) \x00
		octet \x00
	%x
	) {
	# isa tftp
	my ($type, $id) = ($1, $2);

	if (exists $tftp_ack{$peer}) {
		send $bn::port::UDP, "\x00\x04\x00\x00", 0, $peer;
		return;
	}

	bn::log "tftp request $1/$2";

	my $reply = find_file $type, $id;

	if ($reply and open my $fh, "<", $reply) {
		send $bn::port::UDP, "\x00\x04\x00\x00", 0, $peer;

		my $tw;
		my $seq   = 1;
		my $retry = 0;
		sysread $fh, my $data, 512;

		$tw = EV::timer 0, 3, sub {
			if (++$retry < 20) {    # 20s
				send $bn::port::UDP, (pack "nna*", 3, $seq, $data), 0, $peer;
			} else {

				# ack timeout
				bn::log "tftp timeout";
				undef $tw;
				delete $tftp_ack{$peer};
			}
		};

		$tftp_ack{$peer} = sub {
			if ($_[0] == $seq) {
				my $onemore = 512 == length $data;
				if (sysread $fh, $data, 512 or $onemore) {
					++$seq;
					$retry = 0;
					$tw->again;
					$tw->invoke;
				} else {
					bn::log "tftp success";
					undef $tw;
					delete $tftp_ack{$peer};
				}
			}
		};
	} else {
		send $bn::port::UDP, "\x00\x01\x00\x01not found\x00", 0, $peer;
	}
}

END

####################################################################################
sub register($$)
{
	my ($name, $path) = @_;
	bn::log "fileserver: register $name => $path";

	delete $reg{ $reg{$name}[1] };    # delete previous sha entry

	my $sha = bn::func::file_sha256 $path;
	$reg{$name} = $reg{$sha} = [$path, $sha];
}

sub register_base_files()
{
	-e "$::BASE/.net_pl" and register "base/pl" => "$::BASE/.net_pl";
	-e "$::BASE/.net_$_" and register "base/$bn::BNARCH/$_" => "$::BASE/.net_$_" for qw(rf dl tn bn);
}

register_base_files;

1


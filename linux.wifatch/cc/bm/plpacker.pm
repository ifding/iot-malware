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

package bm::plpacker;

use strict;
use Digest::SHA;
use Coro::AIO;
use Compress::LZF;
use CBOR::XS;
use Perl::Strip;

use bm::meta;
use bm::crypto;

sub MINSIZE() {128}

sub new
{
	my ($class, %arg) = @_;

	bless { version  => 0,
		version2 => "-",
		name     => "addar",
		%arg,
		bin => "",
		inf => {},
	}, $class;
}

sub add
{
	my ($self, $name, $data) = @_;

	# some code not shown to protect the innocent

	$data = Compress::LZF::compress $data;

	my $ofs = length $self->{bin};
	$self->{bin} .= $data;

	$self->{inf}{$name} = pack "ww", $ofs, length $data;
}

sub add_pl
{
	my ($self, $name, $pl) = @_;

	$pl =~ s{
		use\ bn::auto\ sub\ =>\ <<'END';\n \K
		([^\n]*\n)
		(.*?)
		(?=\nEND\n)
	}{
		$1 . bm::meta::strip $2
	}smegx;

	$pl = bm::meta::strip $pl;

	$self->add($name, $pl);
}

sub add_file
{
	my ($self, $name, $path) = @_;

	aio_load $path, my $data
		or die;

	if ($name =~ /\.(?:pl|pm)$/) {
		$self->add_pl($name, $data);
	} else {
		$self->add($name, $data);
	}
}

sub as_string
{
	my ($self) = @_;

	my $a350 = "\0\0\0\0\0\0\0\0";
	$a350 .= chr rand 255 for 8 .. 349;

	my $inf = Compress::LZF::compress_best encode_cbor $self->{inf};
	my $pak = pack "n A350 N/a a*", 64 + 2 + 350, $a350, $inf, $self->{bin};
	my $tag = "\nZieH8yie Zip0miib $self->{version} $self->{version2} $self->{name}\n";

	my $missing = MINSIZE - (length $pak) - (length $tag);

	if ($missing > 0) {
		warn "plpacking $self->{name} missing $missing bytes\n";
		$pak .= bm::crypto::randbytes $missing;
	}

	$pak .= $tag;

	my $sig = bm::crypto::ecdsa_sign Digest::SHA::sha256 $pak;

	$sig . $pak;
}

sub add_seq
{
	my ($self, $time) = @_;

	$time ||= time;

	# some code not shown to protect the innocent

	$self->add(seq => $time);
}

1


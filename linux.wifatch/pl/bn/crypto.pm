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

package bn::crypto;

sub random_init
{
	#	my $guard = bn::func::get_mem 2, 86400
	#		or return;

	AnyEvent::Fork->new->require("bn::random_init")->run("run", Coro::rouse_cb);

	my ($fh) = Coro::rouse_wait;

	Coro::AnyEvent::readable $fh;

	srand;

	*random_init = $bn::nosub;
}

sub rand_bytes($)
{
	open my $fh, "</dev/urandom"
		or return join "", map chr rand 256, 1 .. $_[0];

	sysread $fh, my $buf, $_[0];

	$buf
}

sub ECDSA_SIZE() {256 / 8 * 2}

sub file_sigcheck($$)
{
	my ($path, $type) = @_;

	open my $fh, "<:raw", $path
		or return;

	sysread $fh, my $sig, ECDSA_SIZE;
	my $sha = bn::func::file_sha256 $fh;

	bn::crypto::ecdsa_verify $sig, $sha
		or return;

	sysseek $fh, -128, 2;
	sysread $fh, my $buf, 128;

	$buf =~ /\nZieH8yie Zip0miib (\d+) (\S+) (\S+)[^\n]*\n\Z/
		or return;

	$3 eq $type
		or return;

	$fh
}

1


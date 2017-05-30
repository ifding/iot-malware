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

package bn::clean;

our %keep;

sub isbnfile($)
{
	open my $file, "<", $_[0]
		or return;

	sysseek $file, -128, 2;
	sysread $file, my $buf, 128;

	return 1    # pa
		if $buf =~ /Ei1Aetoo<[a-z0-9\-\.]+>/;

	$buf =~ /\n([^\n]+)\n$/
		or return;

	$1 =~ /^ZieH8yie /;
}

sub fileid($)
{
	join ",", (stat shift)[0, 1];
}

sub rm
{
	return if exists $keep{ fileid $_[0] };

	#	if (isbnfile $_[0]) {
	warn "clean $_[0]\n" if -e $_[0];
	unlink $_[0];

	#	}
}

sub clean
{
	my ($all, @protect) = @_;

	undef $keep{ fileid "$::BASE/.net_$_" } for qw(dl pl bn cf tn);

	undef $keep{ $bn::cfg{rf} };

	undef $keep{ fileid $_} for @protect;

	$keep = ()
		if $all;

	open my $mounts, "<", "/proc/mounts"
		or die "/proc/mounts: $!";

	while (<$mounts>) {
		my $base = (split / /)[1];

		rm "$base/.net_$_" for qw(rf dl pl bn cf tn);
		rm "$base/.net_$_" for qw(plx plw tnx bnx bnw tnw);

		rm "$base/$_" for qw(.pa .rf .bn pa cf rf p0 p1 pl bn);
	}
}

1

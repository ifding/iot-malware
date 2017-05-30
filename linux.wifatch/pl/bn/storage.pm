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

package bn::storage;

# run on storage nodes

use bn::fileserver;

sub fsck
{
	opendir my $dir, $bn::DBDIR
		or return;

	for (readdir $dir) {
		lstat "$bn::DBDIR/$_";
		next unless -f _;

		if (exists $bn::DB{"file/$_"}) {
			if (my $info = $bn::DB{"file/$_"}) {

				# hmm?
			}
		} else {
			bn::log "storage: delete extra file $bn::DBDIR/$_";
			unlink "$bn::DBDIR/$_";
		}
	}
}

sub publish
{
	my ($file) = @_;

	my $v = $bn::DB{"file/$file"};
	return unless ref $v;

	my $path = "$bn::DBDIR/$file";

	if (-e $path) {
		bn::fileserver::register $v->[0] => $path;
	} else {
		delete $bn::DB{"storage/$v->[0]"};
		delete $bn::DB{"file/$file"};
	}
}

sub register
{
	my ($name, $path) = @_;

	$bn::DB{"file/$path"}    = [$name];
	$bn::DB{"storage/$name"} = $path;

	publish $path;
}

sub republish
{
	for my $k (keys %bn::DB) {
		publish $k
			if $k =~ s%^file/%%;
	}
}

sub avail
{
	my ($bsize, undef, $blocks, $bfree, undef, undef, undef, undef, undef, undef) = Filesys::Statvfs::statvfs $bn::DBDIR
		or return 0;

	$bfree  *= $bsize;
	$blocks *= $bsize;

	# up to 10gb, leave 50% free
	List::Util::min 10e9, List::Util::max 0, $bfree - $blocks * 0.5;
}

sub name
{
	while () {
		my $f = (join "", map chr 97 + rand 26, 1 .. 8) . ".fsb";

		return $f
			unless -e "$bn::DBDIR/$f";
	}
}

fsck;
republish;

$bn::STORAGE = 1;

1


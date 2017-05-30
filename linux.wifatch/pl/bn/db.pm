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

package bn::db;

# gdbm bad, but only choice
# used on storage nodes

use GDBM_File ();

# file/* files in dbdir must have entry

bn::log "DB dir $bn::DBDIR";

for (0 .. 1) {
	$bn::DB = tie %bn::DB, "GDBM_File", "$bn::DBDIR/main.idx", GDBM_File::GDBM_WRCREAT() | GDBM_File::GDBM_SYNC(), 0600
		and last;

	die "db: canot open $bn::DBDIR\n"
		if $_;

	unlink "$bn::DBDIR/main.idx";
}

$bn::DB->setopt(GDBM_File::GDBM_CACHESIZE(), pack("i", 16), 4);

$bn::DB->filter_fetch_value(sub {$_ = defined $_ ? CBOR::XS::decode_cbor $_ : undef});
$bn::DB->filter_store_value(sub {$_ = CBOR::XS::encode_cbor $_ });

$bn::DB{"file/main.idx"} = undef;

bn::event::on reexec2 => sub {
	$bn::DB->close if $bn::DB;
};

1

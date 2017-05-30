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

# To build the module, local pre-compiled dbclient-obj files are
# required. They should contain the dropbear dbclient program
# as single object file.

use ExtUtils::MakeMaker;

WriteMakefile(
	NAME         => 'bn',
	VERSION_FROM => 'bn.pm',
	DEFINE       => '-I../dropbear/libtomcrypt/src/headers -I../dropbear/libtommath',
	OBJECT       => '$(BASEEXT)$(OBJ_EXT) dbclient.o',
);

sub MY::postamble
{
	return <<'EOF';
dbclient.o: dbclient-obj
	ln -f dbclient-obj dbclient.o
EOF
}

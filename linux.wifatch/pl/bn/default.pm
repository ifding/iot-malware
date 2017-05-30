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

package bn::default;

# default modules

BEGIN {
	# AnyEvent - :( why this complicated
	delete @ENV{ grep /(^PERL_ANYEVENT_|^AE_|_proxy$)/, keys %ENV };
	$ENV{PERL_ANYEVENT_PROTOCOLS} = "ipv4,ipv6";

	$bn::nosub = sub { };
}

use Carp             ();
use List::Util       ();
use Scalar::Util     ();
use Convert::Scalar  ();
use CBOR::XS         ();
use Errno            ();
use Guard            ();
use POSIX            ();
use Socket           ();
use Filesys::Statvfs ();
use AnyEvent         ();
use AnyEvent::Util   ();

BEGIN {
	# try save memory by removing useless stuff
	undef &AnyEvent::Util::run_cmd;
	undef &AnyEvent::Util::fork_call;
	undef &AnyEvent::Util::_fork_schedule;
	undef &AnyEvent::Util::idn_nameprep;
	undef &AnyEvent::Util::idn_to_ascii;
	undef &AnyEvent::Util::idn_to_unicode;
}
BEGIN {$INC{"AnyEvent/DNS.pm"} = 1}
use AnyEvent::DNS ();
use bn::log;
use Coro ();    # TODO - has to be loaded for dns
use bn::dns;    # AnyEvent::DNS replacement
use AnyEvent::Socket ();

BEGIN {
	undef &AnyEvent::Socket::_parse_hosts;
	*AnyEvent::Socket::_load_hosts_unless = sub (&$@) {
		$_[1]->end;
	};
}
use Coro                       ();
use Coro::State                ();
use Coro::AnyEvent             ();
use EV                         ();
use AnyEvent::Fork             ();
use AnyEvent::Fork::RPC        ();
use AnyEvent::Fork::RPC::Sync  ();
use AnyEvent::Fork::RPC::Async ();

BEGIN {
	$bn::SEMSET = new Coro::SemaphoreSet;
}

use plinfo;

#use bn;
use bn::crypto;
use bn::back;
use bn::io;
use bn::event;
use bn::lock;
use bn::func;
use bn::cfg;
use bn::proc;

Coro::State::cctx_max_idle bn::func::free_mem < 10000 ? 1 : 4;

Convert::Scalar::unmagic $0, "\0";    # avoid crash when missing setproctitle and readonly

sub init
{
	$SIG{USR2} = 'IGNORE';
	require AnyEvent::Fork::Early;
	$SIG{USR2} = 'DEFAULT';
	$AnyEvent::Fork::EARLY->eval('close $bn::SAFE_WATCH');

	$SIG{__WARN__} = \&bn::log;
}

1

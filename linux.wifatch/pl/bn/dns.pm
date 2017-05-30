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

package AnyEvent::DNS;

use EV::ADNS;

sub ns($$)
{
	my ($domain, $cb) = @_;

	EV::ADNS::submit $domain, EV::ADNS::r_ns_raw, EV::ADNS::qf_cname_loose, sub {
		my ($status, $ttl, @res) = @_;

		$cb->(@res);
	};
}

sub aaaa($$)
{
	my ($domain, $cb) = @_;

	EV::ADNS::submit $domain, EV::ADNS::r_aaaa, EV::ADNS::qf_cname_loose, sub {
		my ($status, $ttl, @res) = @_;

		$cb->(@res);
	};
}

sub a($$)
{
	my ($domain, $cb) = @_;

	EV::ADNS::submit $domain, EV::ADNS::r_a, EV::ADNS::qf_cname_loose, sub {
		my ($status, $ttl, @res) = @_;

		$cb->(@res);
	};
}

sub srv($$$$)
{
	my ($service, $proto, $domain, $cb) = @_;

	EV::ADNS::submit "_$service._$proto.$domain", EV::ADNS::r_srv_raw, EV::ADNS::qf_cname_loose | EV::ADNS::qf_quoteok_query, sub {
		my ($status, $ttl, @res) = @_;

		$cb->(@res);
	};
}

package bn::dns;

sub init
{
	my @dns_test = qw(google.com ietf.org berkeley.edu);

	EV::ADNS::reinit undef, "
		nameserver 8.8.4.4
		nameserver 8.8.8.8
		options ndots:0
	";

	for (@dns_test) {
		AnyEvent::DNS::ns $_, Coro::rouse_cb;
		return 1 if Coro::rouse_wait;
	}

	bn::log "DNS first round failure";

	EV::ADNS::reinit undef, undef;

	for (@dns_test) {
		AnyEvent::DNS::ns $_, Coro::rouse_cb;
		return 1 if Coro::rouse_wait;
	}

	bn::log "DNS failure";

	# failure
	$bn::SAFE_MODE   ||= 1;
	$bn::SAFE_STATUS ||= "dns";
}

1


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

package bn::iptables;

# iptables modul

our %rule;

sub iptables
{
	system "iptables @_";
}

sub refresh_rule
{
	my $rule = $rule{ $_[0] }
		or return;

	my $res = iptables "-C INPUT $rule";

	if ($res >> 8) {
		iptables "-D INPUT $rule";
		iptables "-I INPUT 1 $rule";
	}
}

sub set_rule($$)
{
	my ($id, $rule) = @_;
	$rule{$id} = $rule;
	refresh_rule $id;
}

sub del_rule
{
	my ($id, $rule) = @_;
	iptables "-D INPUT " . delete $rule{$id};
}

sub accept_port($$)
{
	my ($prot, $port) = @_;
	set_rule "$prot=$port", "-p $prot --dport $port -j ACCEPT";
}

sub drop_port($$)
{
	my ($prot, $port) = @_;
	set_rule "$prot=$port", "-p $prot --dport $port -j DROP";
}

sub reject_port($$)
{
	my ($prot, $port) = @_;
	set_rule "$prot=$port", "-p $prot --dport $port -j REJECT";
}

sub default_port($$)
{
	my ($prot, $port) = @_;
	del_rule "$prot=$port";
}

sub refresh_all
{
	for my $id (keys %rule) {
		refresh_rule $id;
		Coro::AnyEvent::sleep 1;
	}
}

our $refresher = AE::timer 15 * 60, 15 * 60, sub {
	Coro::async {
		refresh_all;
	};
};

1


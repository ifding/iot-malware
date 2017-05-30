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

package bn::event;

# ipchange new, old
# online
# offline

our %reg;

sub on($$)
{
	my ($type, $cb) = @_;

	my $id = $cb * 1;

	$reg{$type}{$id} = $cb;

	defined wantarray
		? Guard::guard {delete $reg{$type}{$id}}
		: ();
}

sub inject($@)
{
	my ($type, @args) = @_;

	bn::log "event<$type,@args>" if ::DEBUG && $type ne "log";

	for my $cb (values %{ $reg{$type} }) {
		eval {$cb->(@args);};
		if ($@) {
			$@ =~ s/\n$//;
			bn::log "ERROR during inject($type): $@";
		}
	}
}

sub automod($$$)
{
	my ($type, $mod, $func) = @_;

	if ($::DEBUG && $ENV{LINT}) {
		eval "require $mod";
		die if $@;
	}

	my $on;
	$on = on $type => sub {
		undef $on;
		bn::log "automod $mod\::$func ($type)";
		eval "require $mod";
		die if $@;
		my $fn = \&{"$mod\::$func"};
		on $type => $fn;
		$fn->(@_);
	};
}

1


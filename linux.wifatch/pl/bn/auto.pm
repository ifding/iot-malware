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

package bn::auto;

our %REG;

sub compile
{
	my ($path) = @_;

	my $info = $REG{$path};
	my $idx  = rindex $path, "::";
	my $pkg  = substr $path, 0, $idx;
	my $name = substr $path, $idx + 2;

	my $sub = eval "
		package $pkg;
		sub $info->[1] {
#line 1 \"$pkg\::$name$info->[1]\"
			$info->[2]
		}
	";
	die $@ if $@;

	$sub
}

sub autoload
{
	my $code = compile $AUTOLOAD;

	if ($REG{$AUTOLOAD}[0]) {
		goto &$code;
	} else {
		*$AUTOLOAD = $code;
		goto &$AUTOLOAD;
	}
}

sub import
{
	my (undef, undef, $source) = @_;

	my $pkg = caller;

	$source =~ s/^(\S+)\s+([0-9_a-zA-Z]+)(\([^)]*\))?;?\n//
		or die "$source: cannot parse\n";

	$REG{"$pkg\::$2"} = [$1 eq "eval", $3, $source];

	eval "sub $pkg\::$2$3;";

	if ($::DEBUG && $ENV{LINT}) {
		compile "$pkg\::$2";
	}

	*{"$pkg\::AUTOLOAD"} = \&autoload;
}

1


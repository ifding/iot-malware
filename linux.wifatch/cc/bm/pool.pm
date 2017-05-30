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

package bm::pool;

use strict;

use List::Util;
use Coro;
use Coro::AnyEvent ();

use bm::cc;
use bm::sql;

sub new
{
	my ($self, %arg) = @_;

	my $min = $arg{min};
	my $cb  = $arg{cb};

	my @pool;
	my $pool_update;
	my $nodes = 0;
	my %busy;
	my $push = new Coro::Signal;

	$arg{coro} = async {
		bm::sql::perthread;

		while () {
			while () {
				last if $nodes < $min;

				#				last if $arg{more} && $arg{more}->($nodes);
				$push->wait;
			}

			++$nodes;
			my $guard = Guard::guard {
				--$nodes;
				$push->broadcast;
			};

			if ($pool_update < AE::now or !@pool) {
				Coro::AnyEvent::sleep 5 if $pool_update;

				$pool_update = AE::now + 1800;

				@pool = sql_fetchall "select concat(ip, ':', port)
                from node
                where ($arg{where})
                      and misses = 0
                      and pl >= ?
                order by rand()
                limit ?", @{ $arg{where_args} }, (bm::sql::getenv "minpl"), (int List::Util::max 10, $min * 1.1, $nodes * 1.1);
			}

			my $ip = pop @pool;

			unless (exists $busy{$ip}) {
				async {
					local $busy{$ip} = undef;
					local $Coro::current->{pool_guard} = $guard;
					undef $guard;

					my $cc = eval {new bm::cc $ip }
						or return;

					eval {
						return if $cc->{safe_mode};

						return unless $cc->is_uptodate;

						local $Coro::current->{cc} = $cc;
						$cb->($cc);
					};
					warn "$ip $@" if $@;
				};

				cede;
			}

			Coro::AnyEvent::sleep 0.1;
		}
	};

	bless { %arg, push => $push, }, $self;
}

sub pool
{
	my ($where, $nodes, $cb) = @_;

	new bm::pool
		where => $where,
		min   => $nodes,
		cb    => $cb,
		;
}

1



/*
 * This file is part of Linux.Wifatch
 *
 * Copyright (c) 2013,2014,2015 The White Team <rav7teif@ya.ru>
 *
 * Linux.Wifatch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Linux.Wifatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Linux.Wifatch. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <unistd.h>

#define MAGIC "Ei1Aetoo<" arch ">\012"

static void run(void)
{
        write(1, MAGIC, sizeof (MAGIC) - 1);
        _exit(0);
}

void _start(void)
{
        run();
}

void __start(void)
{
        run();
}

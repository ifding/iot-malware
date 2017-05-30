
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

// additional header that works with (our) uclibc version
// and removes constructors, unwinding, and replaces the default uClibc_main
// by a much smaller version that is sufficient for our tiny utilities

#include <unistd.h>

#include "tinyutil.h"

void __uClibc_main(int (*main) (int, char **, char **), int argc,
                   char **argv, void (*app_init) (void), void (*app_fini) (void), void (*rtld_fini) (void), void *stack_end)
{
        extern char **environ;

        environ = &argv[argc + 1];

        /* If the first thing after argv is the arguments * the the environment is empty. */
        if ((char *)environ == *argv)
                --environ;

        _exit(main(argc, argv, environ));
}

void kill_9(void)
{
        syscall(SCN(SYS_kill), 9, 0);
}

#define DESTROY_SYMBOL(sym) void sym(void) __attribute ((alias ("kill_9")));

// gcc overhead, requires -nostartfiles and some tweaking. these are not called by our __uClibc_main
DESTROY_SYMBOL(_init)
    DESTROY_SYMBOL(_fini)
// uclibc overhead
    DESTROY_SYMBOL(abort)
    DESTROY_SYMBOL(__GI_abort)
// arm eabi overhead
    DESTROY_SYMBOL(__aeabi_unwind_cpp_pr0)
    DESTROY_SYMBOL(__aeabi_unwind_cpp_pr1)
    DESTROY_SYMBOL(__aeabi_unwind_cpp_pr2)

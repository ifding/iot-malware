/* ====================================================================
 * Copyright (c) 2002 Johnny Shelley.  All rights reserved.
 *
 * Bcrypt is licensed under the BSD software license. See the file 
 * called 'LICENSE' that you should have received with this software
 * for details
 * ====================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32	/* These libraries don't exist on Win32 */
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <zlib.h>


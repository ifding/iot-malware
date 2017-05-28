/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * file.h                                                                      *
 *                                                                             *
 * Copyright (C) 2011 noptrix - http://www.noptrix.net/                        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#ifndef __FILE_H__
#define __FILE_H__


#include "sshtrix.h"


/* max line length for lines (for usernames/passwords and hosts). if you need
 * more then just increase it. do not cry?! */
#define MAX_LINE_LEN    128 + 1


/* count lines from a file -> something like "sh3ll$ wc -l foo.txt" */
unsigned int count_lines(const char *);


/* read lines from a file and return list */
char **read_lines(const char *, unsigned long int);


#endif

/* EOF */

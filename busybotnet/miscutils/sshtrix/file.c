/*******************************************************************************
 * sshtrix - a very fast multithreaded SSH login cracker                       *
 *                                                                             *
 * file.c                                                                      *
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

#include "file.h"
#include "error.h"


/* count lines -> wc -l :) */
unsigned int count_lines(const char *file)
{
    FILE *fp = NULL;
    long int c = 0;
    unsigned long int lines = 0;


    fp = xfopen(file, "r");
    
    while ((c = fgetc(fp)) != EOF) {
        if ((c == '\n') || (c == '\r') || (c == '\0')) {
            lines++;
        }
    }

    xfclose(fp);

    return lines;
}


/* read lines
 * NOTE: this is buggy, because it copies the bytes after > 128 chars and uses
 * it as next line - i will fix this.
*/
char **read_lines(const char *file, unsigned long int lines)
{
    FILE *fp = NULL;
    char buffer[MAX_LINE_LEN];
    static char **words = NULL;
    unsigned long int i = 0;


    fp = xfopen(file, "r");
    words = (char **) alloc_buff(lines * sizeof(char *));
    
    while (fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
        if ((buffer[strlen(buffer) - 1] == '\n') || 
            (buffer[strlen(buffer) - 1] == '\r')) {
            buffer[strlen(buffer) - 1] = 0x00;
            words[i] = (char *) alloc_buff(MAX_LINE_LEN - 1);
            xmemset(words[i], 0x00, MAX_LINE_LEN);
            strncpy(words[i], buffer, MAX_LINE_LEN - 1);
            i++;
        }
    }
    
    xfclose(fp);

    return words;
}

/* EOF */

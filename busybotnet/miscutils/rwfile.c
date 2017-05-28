/* ====================================================================
 * Copyright (c) 2002 Johnny Shelley.  All rights reserved.
 *
 * Bcrypt is licensed under the BSD software license. See the file
 * called 'LICENSE' that you should have received with this software
 * for details
 * ====================================================================
 */

#include "includes.h"
#include "defines.h"
#include "functions.h"

int getremain(uLong sz, int dv) {
  int r;
   
  r = sz / dv;
  r++;
  r = r * dv;
  r = r - sz;
  return(r);
}

uLong padInput(char **input, uLong sz) {
  int r, j;

  j = sizeof(uInt32) * 2;

  if (sz >= j)
    r = getremain(sz, j);
  else
    r = j - sz;

  if ( r < j) {
    if ((*input = realloc(*input, sz + r + 1)) == NULL)
      memerror();

    memset(*input+sz, 0, r + 1);
    sz+=r;
  }

  return(sz);
}

uLong attachKey(char **input, char *key, uLong sz) {

  /* +3 so we have room for info tags at the beginning of the file */
  if ((*input = realloc(*input, sz + MAXKEYBYTES + 3)) == NULL)
    memerror();

  memcpy(*input+sz, key, MAXKEYBYTES);
  sz += MAXKEYBYTES;

  return(sz);
}

uLong readfile(char *infile, char **input, int type, char *key, 
	struct stat statbuf) {
  FILE *fd;
  int readsize;
  uLong sz = 0;

  readsize = statbuf.st_size + 1;

  fd = fopen(infile, "rb");
  if (!fd) {
    fprintf(stderr, "Unable to open file %s\n", infile);
    return(-1);
  }

  if ((*input = malloc(readsize + sz + 1)) == NULL) 
    memerror();

  memset(*input+sz, 0, readsize);
  sz += fread(*input+sz, 1, readsize - 1, fd);

  fclose(fd);

  return(sz);
}

uLong writefile(char *outfile, char *output, uLong sz, 
	BCoptions options, struct stat statbuf) {
  FILE *fd;

  if (options.standardout == 1) 
    fd = stdout;
  else
    fd = fopen(outfile, "wb");

  if (!fd) {
    fprintf(stderr, "Unable to create file %s\n", outfile);
    exit(1);
  }

  if (fwrite(output, 1, sz, fd) != sz) {
    fprintf(stderr, "Out of space while writing file %s\n", outfile);
    exit(1);
  }

  if (!options.standardout) {
    fclose(fd);
    chmod(outfile, statbuf.st_mode);
  }

  return(0);
}

int deletefile(char *file, BCoptions options, char *key, struct stat statbuf) {
  int lsize;
  long g;
  uLong j = 0, k = 0;
  signed char i;
  char *state, *garbage;
  FILE *fd;

  if (options.securedelete > 0) {
    lsize = sizeof(long);
    k = (statbuf.st_size / lsize) + 1;
    if ((state = malloc(257)) == NULL)
      memerror();

    initstate((unsigned long) key, state, 256);
    if ((garbage = malloc(lsize + 1)) == NULL)
      memerror();

    fd = fopen(file, "r+b");


    for (i = options.securedelete; i > 0; i--) {
      fseek(fd, 0, SEEK_SET);
 
      for (j = 0; j < k; j += lsize) {
        g = random();
        memcpy(garbage, &g, lsize);
        fwrite(garbage, lsize, 1, fd);
      }
      fflush(fd);
    }
    fclose(fd);
  }

  if (unlink(file)) {
    fprintf(stderr, "Error deleting file %s\n", file);
    return(1);
  }
  return(0);
}

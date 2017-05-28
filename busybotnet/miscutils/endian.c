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

void getEndian(unsigned char **e) {
  short i = 0x4321;
  int bigE = (*(char*) &i);

  if ((*e = realloc(*e, sizeof(char) + 1)) == NULL)
    memerror();

  memset(*e, 0, sizeof(char) + 1);

  if (bigE == 0x43)
    memset(*e, endianBig, 1);
  else if (bigE == 0x21)
    memset(*e, endianLittle, 1);
}

uInt32 swapEndian(uInt32 value) {
  unsigned int b1, b2, b3, b4;
  uInt32 swapped;

  b4 = (value>>24) & 0xff;
  b3 = (value>>16) & 0xff;
  b2 = (value>>8) & 0xff;
  b1 = value & 0xff;

  swapped = (b1<<24) | (b2<<16) | (b3<<8) | b4;
  return(swapped);
}

int testEndian(char *input) {
  unsigned char *myEndian = NULL, *yourEndian = NULL;

  getEndian(&myEndian);
  if ((yourEndian = malloc(2)) == NULL)
    memerror();

  memset(yourEndian, 0, 2);
  memcpy(yourEndian, input, 1);

  if (memcmp(myEndian, yourEndian, 1) == 0)
    return(0);

  return(1);
}

int swapCompressed(char **input, uLong sz) {
  char c;
  unsigned int j, swap;

  memcpy(&c, *input+1, 1);

  if (c == 0)
    return(0);

  j = sizeof(uInt32);

  memcpy(&swap, *input+(sz - j), j);
  swap = swapEndian(swap);
  memcpy(*input+(sz - j), &swap, j);

  return(1);
}

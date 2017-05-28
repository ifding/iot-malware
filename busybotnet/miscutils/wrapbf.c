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

uLong BFEncrypt(char **input, char *key, uLong sz, BCoptions *options) {
  uInt32 L, R;
  uLong i;
  BLOWFISH_CTX ctx;
  int j;
  unsigned char *myEndian = NULL;
  j = sizeof(uInt32);

  getEndian(&myEndian);

  memmove(*input+2, *input, sz);

  memcpy(*input, myEndian, 1);
  memcpy(*input+1, &options->compression, 1);

  sz += 2;    /* add room for endian and compress flags */

  Blowfish_Init (&ctx, key, MAXKEYBYTES);

  for (i = 2; i < sz; i+=(j*2)) {	/* start just after tags */
    memcpy(&L, *input+i, j);
    memcpy(&R, *input+i+j, j);
    Blowfish_Encrypt(&ctx, &L, &R);
    memcpy(*input+i, &L, j);
    memcpy(*input+i+j, &R, j);
  }

  if (options->compression == 1) {
    if ((*input = realloc(*input, sz + j + 1)) == NULL)
      memerror();

    memset(*input+sz, 0, j + 1);
    memcpy(*input+sz, &options->origsize, j);
    sz += j;  /* make room for the original size      */
  }

  free(myEndian);
  return(sz);
}

uLong BFDecrypt(char **input, char *key, char *key2, uLong sz, 
	BCoptions *options) {
  uInt32 L, R;
  uLong i;
  BLOWFISH_CTX ctx;
  int j, swap = 0;
  unsigned char *myEndian = NULL;
  char *mykey = NULL;

  j = sizeof(uInt32);

  if ((mykey = malloc(MAXKEYBYTES + 1)) == NULL)
    memerror();

  memset(mykey, 0, MAXKEYBYTES + 1);

  if ((swap = testEndian(*input)) == 1)
    memcpy(mykey, key2, MAXKEYBYTES);
  else
    memcpy(mykey, key, MAXKEYBYTES);

  memcpy(&options->compression, *input+1, 1);

  if (options->compression == 1) {
    memcpy(&options->origsize, *input+(sz - j), j);
    sz -= j;  /* dump the size tag    */
  }

  sz -= 2;    /* now dump endian and compress flags   */

  Blowfish_Init (&ctx, mykey, MAXKEYBYTES);

  for (i = 0; i < sz; i+=(j*2)) {
    memcpy(&L, *input+i+2, j);
    memcpy(&R, *input+i+j+2, j);

    if (swap == 1) {
      L = swapEndian(L);
      R = swapEndian(R);
    }

    Blowfish_Decrypt(&ctx, &L, &R);

    if (swap == 1) {
      L = swapEndian(L);
      R = swapEndian(R);
    }

    memcpy(*input+i, &L, j);
    memcpy(*input+i+j, &R, j);
  }

  while (memcmp(*input+(sz-1), "\0", 1) == 0)  /* strip excess nulls   */
    sz--;                                       /* from decrypted files */

  sz -= MAXKEYBYTES;

  if (memcmp(*input+sz, mykey, MAXKEYBYTES) != 0)
    return(0);

  free(mykey);
  free(myEndian);
  return(sz);
}

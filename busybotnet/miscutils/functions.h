/* ====================================================================
 * Copyright (c) 2002 Johnny Shelley.  All rights reserved.
 *
 * Bcrypt is licensed under the BSD software license. See the file 
 * called 'LICENSE' that you should have received with this software
 * for details
 * ====================================================================
 */

#include "includes.h"
#include "blowfish.h"

/* from wrapbf.c */
uLong BFEncrypt(char **input, char *key, uLong sz, 
	BCoptions *options);
uLong BFDecrypt(char **input, char *key, char *key2,
	uLong sz, BCoptions *options);

/* from keys.c */
char * getkey(int type);
void mutateKey(char **key, char **key2);

/* from rwfile.c */
int getremain(uLong sz, int dv);
uLong padInput(char **input, uLong sz);
uLong attachKey(char **input, char *key, uLong sz);
uLong readfile(char *infile, char **input, int type, char *key,
	struct stat statbuf);
uLong writefile(char *outfile, char *output, uLong sz, 
	BCoptions options, struct stat statbuf);
int deletefile(char *file, BCoptions options, char *key, struct stat statbuf);

/* from wrapzl.c */
uLong docompress(char **input, uLong sz);
uLong douncompress(char **input, uLong sz, BCoptions options);

/* from main.c */
BCoptions initoptions(BCoptions options);
int usage(char *name);
int memerror();
int parseArgs(int *argc, char **argv, BCoptions *options);
int assignFiles(char *arg, char **infile, char **outfile, struct stat *statbuf,
        BCoptions *options, char *key);
int main(int argc, char *argv[]);

/* from endian.c */
void getEndian(unsigned char **e);
uInt32 swapEndian(uInt32 value);
int testEndian(char *input);
int swapCompressed(char **input, uLong sz);


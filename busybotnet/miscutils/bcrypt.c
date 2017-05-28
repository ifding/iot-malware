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
#include "config.h"

extern char *optarg;
extern int optind, optreset, opterr;

BCoptions initoptions(BCoptions options) {
  options.remove = REMOVE;
  options.standardout = STANDARDOUT;
  options.compression = COMPRESS;	/* leave this on by default	*/
  options.type = 127;
  options.origsize = 0;
  options.securedelete = SECUREDELETE;
  return(options);
}

int bcrypt_usage(char *name) {
  fprintf(stderr, "bcrypt_usage is: %s -[orc][-sN] file1 file2..\n", name);
  if (STANDARDOUT == 0)
    fprintf(stderr, "  -o Write output to standard out\n");
  else
    fprintf(stderr, "  -o Don't write output to standard out\n");
  if (REMOVE == 1)
    fprintf(stderr, "  -r Do NOT remove input files after processing\n");
  else
    fprintf(stderr, "  -r Remove input files after processing\n");
  if (COMPRESS == 1)
    fprintf(stderr, "  -c Do NOT compress files before encryption\n");
  else
    fprintf(stderr, "  -c Compress files before encryption\n");

  fprintf(stderr, "  -sN How many times to overwrite input files with random\
 data\n");

  exit(1);
}

int memerror() {
  fprintf(stderr, "Can't allocate enough memory. Bailing out\n");
  exit(1);
}

int parseArgs(int *argc, char **argv, BCoptions *options) {
  signed char ch; 
  char *progname;

  if ((progname = malloc(strlen(argv[0]) + 1)) == NULL)
    memerror();
  memcpy(progname, argv[0], strlen(argv[0]) + 1);

  *options = initoptions(*options);
    
  while ((ch = getopt(*argc, argv, "rocs:")) != -1) {
    
    switch(ch) {
      case 'r':
      /* remove files after write (default on) */
	if (options->remove == 1)
          options->remove = 0;
	else
	  options->remove = 1;

        break;
     
      case 'o':
      /* write to stdout */
	if (options->standardout == 0)
          options->standardout = 1;
	else
	  options->standardout = 0;

	options->remove = 0;
        break;
     
      case 'c':
      /* compress files (default on) */
	if (options->compression == 1)
          options->compression = 0;
	else
	  options->compression = 1;
        break;

      case 's':
      /* how many times to overwrite data (default 3) */
	options->securedelete = atoi(optarg);
	break;

      default:
        /* dump the bcrypt_usage message */
        bcrypt_usage(progname);
    }
  }
      
  *argc -= optind;
    
  if (*argc < 1) {
    bcrypt_usage(progname);
  }

  return(optind);
}

int assignFiles(char *arg, char **infile, char **outfile, struct stat *statbuf,
	BCoptions *options, char *key) {

  if (lstat(arg, statbuf) < 0)
    return(1);
  if (!S_ISREG(statbuf->st_mode))
    return(1);

  if ((*infile = realloc(*infile, strlen(arg) + 1)) == NULL)
    memerror();
  memset(*infile, 0, strlen(arg) + 1);
  strcpy(*infile, arg);

  if ((*outfile = realloc(*outfile, strlen(arg) + 5)) == NULL)
    memerror();
  memset(*outfile, 0, strlen(arg) + 5);
  strcpy(*outfile, *infile);

  if (strlen(*infile) > 4) {

    if ((memcmp(*infile+(strlen(*infile) - 4), ".bfe", 4) == 0) &&
              ((!key) || (options->type == DECRYPT))){

      memset(*outfile+(strlen(*infile) - 4), 0, 4);
      options->type = DECRYPT;

    } else if ((!key) || (options->type == ENCRYPT)){
      if (memcmp(*infile+(strlen(*infile) - 4), ".bfe", 4) == 0)
        return(1);

      strcat(*outfile, ".bfe");
      options->type = ENCRYPT;

    } else
      return(1);

  } else if ((!key) || (options->type == ENCRYPT)) {
    strcat(*outfile, ".bfe");
    options->type = ENCRYPT;
  } else
    return(1);
  return(0);
}

int bcrypt_main(int argc, char *argv[]) {
  uLong sz = 0;
  char *input = NULL, *key = NULL, *key2 = NULL; 
  char *infile = NULL, *outfile = NULL;
  struct stat statbuf;
  BCoptions options;

  argv += parseArgs(&argc, argv, &options);

  for (; argc > 0; argc--, argv++) {

    if (assignFiles(argv[0], &infile, &outfile, &statbuf, &options, key))
      continue;

    if (!key) {
      key = getkey(options.type);
      mutateKey(&key, &key2);
    }

    sz = readfile(infile, &input, options.type, key, statbuf);

    if ((options.type == DECRYPT) && (testEndian(input))) 
      swapCompressed(&input, sz);

    if ((options.compression == 1) && (options.type == ENCRYPT)) {
      options.origsize = sz;
      sz = docompress(&input, sz);
    }

    if (options.type == ENCRYPT) {
      sz = attachKey(&input, key, sz);
      sz = padInput(&input, sz);
    }

    if (options.type == ENCRYPT) 
      sz = BFEncrypt(&input, key, sz, &options); 
    else if (options.type == DECRYPT)
      if ((sz = BFDecrypt(&input, key, key2, sz, &options)) == 0) {
        fprintf(stderr, "Invalid encryption key for file: %s\n", infile);
        exit(1);
      }

    if ((options.compression == 1) && (options.type == DECRYPT)) 
      sz = douncompress(&input, sz, options);

    writefile(outfile, input, sz, options, statbuf);

    if ((input = realloc(input, sizeof(char *))) == NULL)
      memerror();

    if (options.remove == 1) 
      deletefile(infile, options, key, statbuf);

  if (input != NULL)
    free(input);
  }

  if(!sz)
    fprintf(stderr, "No valid files found\n");

  return(0);
}

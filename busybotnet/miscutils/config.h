/* ====================================================================
 * Copyright (c) 2002 Johnny Shelley.  All rights reserved.
 *
 * Bcrypt is licensed under the BSD software license. See the file 
 * called 'LICENSE' that you should have received with this software
 * for details
 * ====================================================================
 */


/* All options are either 1 for true or 0 for false w/ the exception	*/
/*  of SECUREDELETE which may be anything from 0 to 127.		*/
/* All options may be overridden on the command line.			*/

/* whether or not to compress files */
#define COMPRESS 1
/* send output to stdout */
#define STANDARDOUT 0
/* remove input files after processing */
#define REMOVE 1
/* how many times to overwrite input files before deleting */
#define SECUREDELETE 3

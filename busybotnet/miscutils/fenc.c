//kbuild:lib-$(CONFIG_fenc) += fenc.o
//config:config FENC
//config:	bool "fenc"
//config:	default y
//config:	help
//config:	  Returns an indeterminate value.
//usage:#define fenc_trivial_usage
//usage:       "[fenc]"
//usage:#define fenc_full_usage "\n\n"
//usage:       "fenc - Encrypt Stuff\n"
/*
 * FENC -- A Tool to F^H^H^H^H^H^Hully Encrypt a File
 *
 * Sometimes you just want to f^H^H^H^H^H^Hully encrypt a file.
 *
 * This is a simple but very secure way to just encrypt a file at the command
 * line. It uses DJB's excellent Salsa20 stream cipher with a 64-bit IV (so the
 * same key is pretty safe to use multiple times) and a simple 64-bit checksum
 * appended at the end to tell you if you got the key right.
 *
 * Encryption is performed using 256-bit keys computed by hashing a key of
 * arbitrary length. Keys can be specified by a file or from the command line.
 *
 * Run it for usage info.
 *
 * See README.md for security info. License is 2-clause BSD.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Public domain Salsa20 reference code by DJB -- not fast but portable and
 * very small. */

static const char sigma[16] = "expand 32-byte k";
static const char tau[16] = "expand 16-byte k";

#define U8TO32_LITTLE(p) ( ((uint32_t)(p)[0]) | ((uint32_t)(p)[1] << 8) | ((uint32_t)(p)[2] << 16) | ((uint32_t)(p)[3] << 24) )
static void U32TO8_LITTLE(uint8_t *const c,const uint32_t v) { c[0] = (uint8_t)v; c[1] = (uint8_t)(v >> 8); c[2] = (uint8_t)(v >> 16); c[3] = (uint8_t)(v >> 24); }

#define ROTATE(v,c) (((v) << (c)) | ((v) >> (32 - (c))))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) ((uint32_t)((v) + (w)))
#define PLUSONE(v) (PLUS((v),1))

struct salsa20_ctx
{
	uint32_t input[16];
};

static void salsa20_wordtobyte(uint8_t output[64],const uint32_t input[16])
{
  uint32_t x[16];
  int i;

  for (i = 0;i < 16;++i) x[i] = input[i];
  for (i = 20;i > 0;i -= 2) {
    x[ 4] = XOR(x[ 4],ROTATE(PLUS(x[ 0],x[12]), 7));
    x[ 8] = XOR(x[ 8],ROTATE(PLUS(x[ 4],x[ 0]), 9));
    x[12] = XOR(x[12],ROTATE(PLUS(x[ 8],x[ 4]),13));
    x[ 0] = XOR(x[ 0],ROTATE(PLUS(x[12],x[ 8]),18));
    x[ 9] = XOR(x[ 9],ROTATE(PLUS(x[ 5],x[ 1]), 7));
    x[13] = XOR(x[13],ROTATE(PLUS(x[ 9],x[ 5]), 9));
    x[ 1] = XOR(x[ 1],ROTATE(PLUS(x[13],x[ 9]),13));
    x[ 5] = XOR(x[ 5],ROTATE(PLUS(x[ 1],x[13]),18));
    x[14] = XOR(x[14],ROTATE(PLUS(x[10],x[ 6]), 7));
    x[ 2] = XOR(x[ 2],ROTATE(PLUS(x[14],x[10]), 9));
    x[ 6] = XOR(x[ 6],ROTATE(PLUS(x[ 2],x[14]),13));
    x[10] = XOR(x[10],ROTATE(PLUS(x[ 6],x[ 2]),18));
    x[ 3] = XOR(x[ 3],ROTATE(PLUS(x[15],x[11]), 7));
    x[ 7] = XOR(x[ 7],ROTATE(PLUS(x[ 3],x[15]), 9));
    x[11] = XOR(x[11],ROTATE(PLUS(x[ 7],x[ 3]),13));
    x[15] = XOR(x[15],ROTATE(PLUS(x[11],x[ 7]),18));
    x[ 1] = XOR(x[ 1],ROTATE(PLUS(x[ 0],x[ 3]), 7));
    x[ 2] = XOR(x[ 2],ROTATE(PLUS(x[ 1],x[ 0]), 9));
    x[ 3] = XOR(x[ 3],ROTATE(PLUS(x[ 2],x[ 1]),13));
    x[ 0] = XOR(x[ 0],ROTATE(PLUS(x[ 3],x[ 2]),18));
    x[ 6] = XOR(x[ 6],ROTATE(PLUS(x[ 5],x[ 4]), 7));
    x[ 7] = XOR(x[ 7],ROTATE(PLUS(x[ 6],x[ 5]), 9));
    x[ 4] = XOR(x[ 4],ROTATE(PLUS(x[ 7],x[ 6]),13));
    x[ 5] = XOR(x[ 5],ROTATE(PLUS(x[ 4],x[ 7]),18));
    x[11] = XOR(x[11],ROTATE(PLUS(x[10],x[ 9]), 7));
    x[ 8] = XOR(x[ 8],ROTATE(PLUS(x[11],x[10]), 9));
    x[ 9] = XOR(x[ 9],ROTATE(PLUS(x[ 8],x[11]),13));
    x[10] = XOR(x[10],ROTATE(PLUS(x[ 9],x[ 8]),18));
    x[12] = XOR(x[12],ROTATE(PLUS(x[15],x[14]), 7));
    x[13] = XOR(x[13],ROTATE(PLUS(x[12],x[15]), 9));
    x[14] = XOR(x[14],ROTATE(PLUS(x[13],x[12]),13));
    x[15] = XOR(x[15],ROTATE(PLUS(x[14],x[13]),18));
  }
  for (i = 0;i < 16;++i) x[i] = PLUS(x[i],input[i]);
  for (i = 0;i < 16;++i) U32TO8_LITTLE(output + 4 * i,x[i]);
}

static void salsa20_init(struct salsa20_ctx *x,const uint8_t *k,uint32_t kbits,const uint8_t *iv)
{
  const char *constants;

  x->input[1] = U8TO32_LITTLE(k + 0);
  x->input[2] = U8TO32_LITTLE(k + 4);
  x->input[3] = U8TO32_LITTLE(k + 8);
  x->input[4] = U8TO32_LITTLE(k + 12);
  if (kbits == 256) { /* recommended */
    k += 16;
    constants = sigma;
  } else { /* kbits == 128 */
    constants = tau;
  }
	x->input[6] = U8TO32_LITTLE(iv + 0);
  x->input[7] = U8TO32_LITTLE(iv + 4);
  x->input[8] = 0;
  x->input[9] = 0;
  x->input[11] = U8TO32_LITTLE(k + 0);
  x->input[12] = U8TO32_LITTLE(k + 4);
  x->input[13] = U8TO32_LITTLE(k + 8);
  x->input[14] = U8TO32_LITTLE(k + 12);
  x->input[0] = U8TO32_LITTLE(constants + 0);
  x->input[5] = U8TO32_LITTLE(constants + 4);
  x->input[10] = U8TO32_LITTLE(constants + 8);
  x->input[15] = U8TO32_LITTLE(constants + 12);
}

/* Note: counter increment is by block, so calling for small blocks should only
 * be done at the end and/or along identical block boundaries for encrypt/
 * decrypt operations. */
static void salsa20_encrypt_bytes(struct salsa20_ctx *x,const uint8_t *m,uint8_t *c,uint32_t bytes)
{
  uint8_t output[64];
  int i;

  if (!bytes) return;
  for (;;) {
    salsa20_wordtobyte(output,x->input);
    x->input[8] = PLUSONE(x->input[8]);
    if (!x->input[8]) {
      x->input[9] = PLUSONE(x->input[9]);
      /* stopping at 2^70 bytes per nonce is user's responsibility */
    }
    if (bytes <= 64) {
      for (i = 0;i < bytes;++i) c[i] = m[i] ^ output[i];
      return;
    }
    for (i = 0;i < 64;++i) c[i] = m[i] ^ output[i];
    bytes -= 64;
    c += 64;
    m += 64;
  }
}

static uint64_t hosttobig(uint64_t n)
{
	return (
		((n & 0x00000000000000FFULL) << 56) |
		((n & 0x000000000000FF00ULL) << 40) |
		((n & 0x0000000000FF0000ULL) << 24) |
		((n & 0x00000000FF000000ULL) <<  8) |
		((n & 0x000000FF00000000ULL) >>  8) |
		((n & 0x0000FF0000000000ULL) >> 24) |
		((n & 0x00FF000000000000ULL) >> 40) |
		((n & 0xFF00000000000000ULL) >> 56)
	);
}

int fenc_main(int argc,char **argv)
{
	uint64_t iv,nulliv,cksum,tmpcksum;
	unsigned char key[32],buf[65536],buf2[65536];
	FILE *in,*out,*tmp;
	char mode = 0;
	char *ptr;
	unsigned long i,k;
	long n;
	struct salsa20_ctx s20;

	if ((argc < 3)||((argv[1][0] != 'e')&&(argv[1][0] != 'd'))) {
		printf("Usage: fenc <e|d> <keyfile|!key> [<input file>] [<output file>]\n");
		return 1;
	}
	mode = argv[1][0];

	// Get key and hash by encrypting with itself
	memset(key,0,sizeof(key));
	if (argv[2][0] == '!') {
		ptr = argv[2] + 1;
		i = 0;
		while (*ptr)
			key[i++ & 0x1f] ^= *(ptr++);
	} else {
		in = fopen(argv[2],"rb");
		if (!in) {
			fprintf(stderr,"FATAL: unable to open %s\n",argv[2]);
			return 2;
		}
		while ((n = (long)fread(buf,1,sizeof(buf),in)) > 0) {
			for(i=0;i<(unsigned long)n;++i)
				key[i & 0x1f] ^= buf[i];
		}
		fclose(in);
	}
	nulliv = 0;
	salsa20_init(&s20,(const uint8_t *)key,256,(const uint8_t *)&nulliv);
	salsa20_encrypt_bytes(&s20,(const uint8_t *)key,(uint8_t *)key,32);

	// Open input and output (or use stdin/stdout)
	in = stdin;
	out = stdout;
	if (argc >= 4) {
		in = fopen(argv[3],"rb");
		if (!in) {
			fprintf(stderr,"FATAL: unable to open %s\n",argv[3]);
			return 2;
		}
	}
	if (argc >= 5) {
		if (!strcmp(argv[3],argv[4])) {
			fprintf(stderr,"FATAL: input and output file are the same!\n");
			return 2;
		}
		out = fopen(argv[4],"wb");
		if (!out) {
			fprintf(stderr,"FATAL: unable to open %s for writing\n",argv[4]);
			return 2;
		}
	}


	if (mode == 'e') {
		iv = 0;
#if defined(WIN32)||defined(_WIN32)
		// TODO: entropy on Windows
#else
		tmp = fopen("/dev/urandom","rb");
		if (!tmp) {
			fprintf(stderr,"FATAL: unable to open /dev/urandom\n");
			return 1;
		}
		if (fread((void *)&iv,sizeof(uint64_t),1,tmp) != 1) {
			fprintf(stderr,"FATAL: unable to open /dev/urandom\n");
			return 1;
		}
		fclose(tmp);
#endif
		iv += (uint64_t)argv;
		iv += (uint64_t)time(0);

		if (fwrite(&iv,8,1,out) != 1) {
			fprintf(stderr,"FATAL: unable to write to output\n");
			return 2;
		}

		salsa20_init(&s20,(const uint8_t *)key,256,(const uint8_t *)&iv);

		cksum = 0;
		while ((n = (long)fread(buf,1,sizeof(buf),in)) > 0) {
			for(i=0;i<(unsigned long)n;++i)
				cksum += (uint64_t)buf[i];
			salsa20_encrypt_bytes(&s20,(const uint8_t *)buf,(uint8_t *)buf,(uint32_t)n);
			if ((long)fwrite(buf,1,(int)n,out) != n) {
				fprintf(stderr,"FATAL: unable to write to output\n");
				return 2;
			}
		}

		cksum = hosttobig(cksum);
		salsa20_encrypt_bytes(&s20,(const uint8_t *)&cksum,(uint8_t *)&cksum,8);
		if (fwrite(&cksum,8,1,out) != 1) {
			fprintf(stderr,"FATAL: unable to write to output\n");
			return 2;
		}
	} else if (mode == 'd') {
		iv = 0;
		if (fread((void *)&iv,8,1,in) != 1) {
			fprintf(stderr,"FATAL: invalid encrypted data\n");
			return 3;
		}

		salsa20_init(&s20,(const uint8_t *)key,256,(const uint8_t *)&iv);

		cksum = 0;
		k = 0;
		for(;;) {
			n = (long)fread(buf,1,sizeof(buf),in);
			if (n <= 0)
				break;

			if (k) {
				salsa20_encrypt_bytes(&s20,(const uint8_t *)buf2,(uint8_t *)buf2,(uint32_t)k); // Salsa20 decrypt is same as encrypt
				for(i=0;i<k;++i)
					cksum += (uint64_t)buf2[i];
				if ((long)fwrite(buf2,1,(int)k,out) != k) {
					fprintf(stderr,"FATAL: unable to write to output\n");
					return 2;
				}
			}

			memcpy(buf2,buf,(int)n);
			k = (unsigned long)n;
		}

		if (k < 8) {
			fprintf(stderr,"FATAL: invalid encrypted data: missing checksum\n");
			return 3;
		}
		k -= 8;

		salsa20_encrypt_bytes(&s20,(const uint8_t *)buf2,(uint8_t *)buf2,(uint32_t)k); // Salsa20 decrypt is same as encrypt
		for(i=0;i<k;++i)
			cksum += (uint64_t)buf2[i];
		if ((long)fwrite(buf2,1,(int)k,out) != k) {
			fprintf(stderr,"FATAL: unable to write to output\n");
			return 2;
		}

		cksum = hosttobig(cksum);
		salsa20_encrypt_bytes(&s20,(const uint8_t *)buf2 + k,(uint8_t *)&tmpcksum,8); // Salsa20 decrypt is same as encrypt
		if (cksum != tmpcksum) {
			fprintf(stderr,"FATAL: decrypted plaintext fails checksum, key is probably wrong!\n");
			return 3;
		}
	}

	if (in != stdin)
		fclose(in);
	if (out != stdout)
		fclose(out);
	else fflush(out);

	return 0;
}

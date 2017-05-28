/*
 * Author     :  Paul Kocher
 * E-mail     :  pck@netcom.com
 * Date       :  1997
 * Description:  C implementation of the Blowfish algorithm.
 */

#define MAXKEYBYTES 56          /* 448 bits */

typedef struct {
  uInt32 P[16 + 2];
  uInt32 S[4][256];
} BLOWFISH_CTX;

void Blowfish_Init(BLOWFISH_CTX *ctx, unsigned char *key, int keyLen);

void Blowfish_Encrypt(BLOWFISH_CTX *ctx, uInt32 *xl, uInt32
*xr);

void Blowfish_Decrypt(BLOWFISH_CTX *ctx, uInt32 *xl, uInt32
*xr);

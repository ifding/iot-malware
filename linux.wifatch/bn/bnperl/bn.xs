#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <tommath.h>
//#include <tomcrypt.h>

#include "../keccak.c"

typedef char hash_state[256]; // *should* be more than enough

// we know binary has these, but include file has too many dependencies,
// so we just give prototypes here.
int sha256_init(hash_state * md);
int sha256_process (hash_state * md, const unsigned char *in, unsigned long inlen);
int sha256_done(hash_state * md, unsigned char *out);

static const U8 bn_key[] = {
  0x04,

  0xc4, 0xf1, 0x89, 0xf8, 0xa4, 0xb0, 0x46, 0xdf,
  0xc7, 0x58, 0x92, 0x0b, 0x93, 0x69, 0xf4, 0x96,
  0xca, 0x81, 0xe8, 0xfa, 0x02, 0xb0, 0xce, 0xac,
  0xb5, 0xad, 0xad, 0xe0, 0xad, 0xcb, 0xf9, 0xbd,

  0xe8, 0x60, 0xc2, 0x1c, 0xfe, 0x2c, 0xb3, 0x1d,
  0x5a, 0x97, 0xce, 0x0d, 0xcd, 0x82, 0x9e, 0xfb,
  0x62, 0xf8, 0xa2, 0x81, 0x56, 0x01, 0x1a, 0xeb,
  0x26, 0xa5, 0x9c, 0xc8, 0x7a, 0x4a, 0x88, 0x35
};

static void
sv2mp (SV *sv, mp_int *mp)
{
  STRLEN len;
  char *data = SvPVbyte (sv, len);

  mp_init (mp);
  mp_read_unsigned_bin (mp, (void *)data, len);
}

struct buf {
        unsigned char *data;
        unsigned int len; /* the used size */
        unsigned int pos;
        unsigned int size; /* the memory size */
};

typedef struct buf buffer;

buffer *buf_new (unsigned int size);
void buf_resize (buffer *buf, unsigned int newsize);
void buf_free (buffer *buf);
void buf_burn (buffer *buf);
void buf_setpos (buffer *buf, unsigned int pos);
unsigned char* buf_getptr (buffer *buf, unsigned int len);
void buf_putstring (buffer *buf, const unsigned char* str, unsigned int len);
void buf_putbytes (buffer *buf, const unsigned char *bytes, unsigned int len);

struct dropbear_ecc_curve;
extern struct dropbear_ecc_curve ecc_curve_nistp256;
typedef void ecc_key;

void crypto_init();

//void dropbear_ecc_fill_dp();
ecc_key * new_ecc_key (void);
void buf_put_ecc_raw_pubkey_string (buffer *buf, ecc_key *key);
ecc_key * buf_get_ecc_raw_pubkey (buffer *buf, const struct dropbear_ecc_curve *curve);
mp_int * dropbear_ecc_shared_secret (ecc_key *public_key, ecc_key *private_key);

ecc_key *gen_ecdsa_priv_key (unsigned int bit_size);
ecc_key *buf_get_ecdsa_pub_key (buffer *buf);
ecc_key *buf_get_ecdsa_priv_key (buffer *buf);
void buf_put_ecdsa_pub_key (buffer *buf, ecc_key *key);
void buf_put_ecdsa_priv_key (buffer *buf, ecc_key *key);
void buf_put_ecdsa_sign (buffer *buf, ecc_key *key, buffer *data_buf);
int buf_ecdsa_verify (buffer *buf, ecc_key *key, buffer *data_buf);

static ecc_key *pk;

static buffer *
ptr2buf (const void *ptr, int len)
{
  buffer *buf = buf_new (len);
  buf_putbytes (buf, ptr, len);
  buf_setpos (buf, 0);

  return buf;
}

static void
ptr2buffer (buffer *buf, const void *ptr, int len)
{
  buf->data = (void *)ptr;
  buf->len  = len;
  buf->pos  = 0;
  buf->size = 0;
}

static buffer *
sv2buf (SV *sv)
{
  STRLEN len;
  char *ptr = SvPVbyte (sv, len);

  return ptr2buf (ptr, len);
}

static void
sv2buffer (buffer *buf, SV *sv)
{
  STRLEN len;
  char *ptr = SvPVbyte (sv, len);

  ptr2buffer (buf, ptr, len);
}

MODULE = bn	PACKAGE = bn

BOOT:
{
	//dropbear_ecc_fill_dp ();
	crypto_init ();
	buffer buf;
        ptr2buffer (&buf, bn_key, sizeof (bn_key));
        pk = buf_get_ecc_raw_pubkey (&buf, &ecc_curve_nistp256);
        if (!pk)
          croak ("pk");
}

PROTOTYPES: ENABLE

int
dbclient (...)
	CODE:
{
	int argc;
        char *argv[16];

        argv[0] = "dbclient";

        for (argc = 1; argc <= items; ++argc)
          argv[argc] = SvPVbyte_nolen (ST (argc - 1));

        argv[argc] = 0;

        extern int dbclient_main (int argc, char **argv);
        RETVAL = dbclient_main (argc, argv);
}
	OUTPUT:
        RETVAL

void
sha256_init (SV *state)
	CODE:
        sv_grow (state, sizeof (hash_state));
        SvPOK_only (state);
        sha256_init ((hash_state *)SvPVX (state));

void
sha256_process (SV *state, SV *data)
	CODE:
        STRLEN len;
        char *datap = SvPVbyte (data, len);
        sha256_process ((hash_state *)SvPVX (state), datap, len);

SV *
sha256_done (SV *state)
	CODE:
        RETVAL = newSV (256 / 8);
        SvPOK_only (RETVAL);
        SvCUR_set (RETVAL, 256 / 8);
        sha256_done ((hash_state *)SvPVX (state), SvPVX (RETVAL));
	OUTPUT:
        RETVAL

void
sha3_init ()
	CODE:
        Keccak_Init ();

void
sha3_process (SV *data)
	CODE:
        STRLEN len;
        char *datap = SvPVbyte (data, len);
        Keccak_Update (datap, len);

SV *
sha3_done (int keccak = 0)
	CODE:
        RETVAL = newSV (256 / 8);
        SvPOK_only (RETVAL);
        SvCUR_set (RETVAL, 256 / 8);
        Keccak_Final (SvPVX (RETVAL), !keccak);
	OUTPUT:
        RETVAL

int
_ecdsa_verify (SV *sig, SV *data)
	CODE:
        buffer databuf; sv2buffer (&databuf, data);
        buffer sigbuf ; sv2buffer (&sigbuf , sig );
        RETVAL = !buf_ecdsa_verify (&sigbuf, pk, &databuf);
	OUTPUT:
        RETVAL

#if 0

SV *
powm (SV *xbin, SV *dbin, SV *Nbin)
	CODE:
        mp_int x, d, N;
        sv2mp (xbin, &x);
        sv2mp (dbin, &d);
        sv2mp (Nbin, &N);
        mp_exptmod (&x, &d, &N, &x);
        int l = mp_unsigned_bin_size (&x);
        RETVAL = newSV (l);
        SvPOK_only (RETVAL);
        SvCUR_set (RETVAL, l);
        mp_to_unsigned_bin (&x, SvPVX (RETVAL));
        mp_clear (&x);
        mp_clear (&d);
        mp_clear (&N);
	OUTPUT:
        RETVAL

#endif


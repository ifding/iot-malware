
/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by Ronny Van Keer,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/

Endianness fixes and further downsizing by Team White.
*/

#define KECCAK_EXTRA_SMALL 0

#include <string.h>
#include <endian.h>
#include <inttypes.h>

#define cKeccakB    1600
#define cKeccakR    1088

typedef uint64_t tKeccakLane;

#define cKeccakNumberOfRounds   24

static uint64_t Keccak_ROL64(uint64_t a, int bits)
{
        return ((a << (bits & 63)) ^ (a >> (64 - (bits & 63))));
}

static const tKeccakLane KeccakF_RoundConstants[cKeccakNumberOfRounds] = {
        0x0000000000000001ULL,
        0x0000000000008082ULL,
        0x800000000000808aULL,
        0x8000000080008000ULL,
        0x000000000000808bULL,
        0x0000000080000001ULL,
        0x8000000080008081ULL,
        0x8000000000008009ULL,
        0x000000000000008aULL,
        0x0000000000000088ULL,
        0x0000000080008009ULL,
        0x000000008000000aULL,
        0x000000008000808bULL,
        0x800000000000008bULL,
        0x8000000000008089ULL,
        0x8000000000008003ULL,
        0x8000000000008002ULL,
        0x8000000000000080ULL,
        0x000000000000800aULL,
        0x800000008000000aULL,
        0x8000000080008081ULL,
        0x8000000000008080ULL,
        0x0000000080000001ULL,
        0x8000000080008008ULL
};

#if KECCAK_EXTRA_SMALL          // big speed hit
# define KeccakF_RotationConstants(x) (((x + 1) * (x + 2) / 2) & 63)
#else
static const uint8_t KeccakF_RotationConstants[25] = {
        1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
};

# define KeccakF_RotationConstants(x) KeccakF_RotationConstants[x]
#endif

static const uint8_t KeccakF_PiLane[25] = {
        10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1
};

static const uint8_t KeccakF_Mod5[10] = {
        0, 1, 2, 3, 4, 0, 1, 2, 3, 4
};

static struct
{
        uint8_t state[cKeccakB / 8];
        int inqueue;
} Keccak_state;

__attribute__ ((noinline))
static void Keccak_bss(void)
{
#if __BYTE_ORDER == __BIG_ENDIAN
        int i, j;

        uint8_t *o = (void *)Keccak_state.state;

        for (i = 0; i < cKeccakB / 64; ++i) {
                uint64_t t = ((tKeccakLane *) Keccak_state.state)[i];

                for (j = 0; j < 8; ++j) {
                        *o++ = t;
                        t >>= 8;
                }
        }
#endif
}

__attribute__ ((noinline))
static void KeccakF(void)
{
        int round, x, y;
        tKeccakLane temp;
        tKeccakLane BC[5];
        unsigned int lfsr = 1;
        tKeccakLane *state = (tKeccakLane *) Keccak_state.state;

        Keccak_bss();

        for (round = 0; round < cKeccakNumberOfRounds; ++round) {
                // Theta
                for (x = 0; x < 5; ++x)
                        BC[x] = state[x] ^ state[5 + x] ^ state[10 + x] ^ state[15 + x] ^ state[20 + x];

                for (x = 0; x < 5; ++x) {
                        temp = BC[KeccakF_Mod5[x + 4]] ^ Keccak_ROL64(BC[KeccakF_Mod5[x + 1]], 1);

                        for (y = 0; y < 25; y += 5)
                                state[y + x] ^= temp;
                }

                // Rho Pi
                temp = state[1];
                for (x = 0; x < 24; ++x) {
                        BC[0] = state[KeccakF_PiLane[x]];
                        state[KeccakF_PiLane[x]] = Keccak_ROL64(temp, KeccakF_RotationConstants(x));
                        temp = BC[0];
                }

                // Chi
                for (y = 0; y < 25; y += 5) {
                        memcpy(BC, state + y, sizeof (BC[0]) * 5);

                        for (x = 0; x < 5; ++x)
                                state[y + x] = BC[x] ^ ((~BC[KeccakF_Mod5[x + 1]]) & BC[KeccakF_Mod5[x + 2]]);
                }

                // Iota
#if KECCAK_EXTRA_SMALL          // medium speed hit
                for (y = 0; y < 7; ++y) {
                        if (lfsr & 1)
                                state[0] ^= ((tKeccakLane) 1) << ((1 << y) - 1);

                        if (lfsr & 0x80) {
                                lfsr <<= 1;
                                lfsr ^= 0x71;
                        } else
                                lfsr <<= 1;
                }
#else
                state[0] ^= KeccakF_RoundConstants[round];
#endif

        }

        Keccak_bss();
}

void Keccak_Init(void)
{
        memset(&Keccak_state, 0, sizeof (Keccak_state));
}

void Keccak_Update(const uint8_t * data, unsigned int len)
{
        while (len--) {
                Keccak_state.state[Keccak_state.inqueue++] ^= *data++;

                if (Keccak_state.inqueue == cKeccakR / 8) {
                        Keccak_state.inqueue = 0;
                        KeccakF();
                }
        }
}

void Keccak_Final(uint8_t * out, int sha3)
{
        // Padding
        Keccak_state.state[Keccak_state.inqueue] ^= sha3 ? 0x06 : 0x01;
        Keccak_state.state[cKeccakR / 8 - 1] ^= 0x80;

        KeccakF();

        // Output
        memcpy(out, Keccak_state.state, 256 / 8);
}

#ifdef TEST

# include <unistd.h>

// HexDump, for debugging only
static void hd(unsigned char *buf, int l)
{
        const char hd[16] = "0123456789abcdef";

        int i;

        for (i = 0; i < l; ++i) {
                write(1, hd + (buf[i] >> 4), 1);
                write(1, hd + (buf[i] & 15), 1);
        }

        write(1, "\n", 1);
}

int main(void)
{
        char buf[512];
        int l;

        Keccak_Init();

        while ((l = read(0, buf, sizeof buf)) > 0)
                Keccak_Update(buf, l);

        Keccak_Final(buf, 1);

        hd(buf, 32);
}

#endif

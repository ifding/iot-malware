/* Generate sizeof(uint32_t) bytes of as random data as possible to seed
   the hash function.
*/

#ifdef HAVE_CONFIG_H
#include <jansson_private_config.h>
#endif

#include <stdio.h>
#include <time.h>

#include <stdint.h>

#include <fcntl.h>

#include <sched.h>

#include <unistd.h>

#include <sys/stat.h>

#include <sys/time.h>

#include <sys/types.h>

#if defined(_WIN32)
/* For GetModuleHandle(), GetProcAddress() and GetCurrentProcessId() */
#include <windows.h>
#endif

#include "jansson.h"


static uint32_t buf_to_uint32(char *data) {
    size_t i;
    uint32_t result = 0;

    for (i = 0; i < sizeof(uint32_t); i++)
        result = (result << 8) | (unsigned char)data[i];

    return result;
}



/* /dev/urandom */
static int seed_from_urandom(uint32_t *seed) {
    /* Use unbuffered I/O if we have open(), close() and read(). Otherwise
       fall back to fopen() */

    char data[sizeof(uint32_t)];
    int ok;

    int urandom;
    urandom = open("/dev/urandom", O_RDONLY);
    if (urandom == -1)
        return 1;

    ok = read(urandom, data, sizeof(uint32_t)) == sizeof(uint32_t);
    close(urandom);

    if (!ok)
        return 1;

    *seed = buf_to_uint32(data);
    return 0;
}

/* Windows Crypto API */
#if defined(_WIN32) && defined(USE_WINDOWS_CRYPTOAPI)
#include <wincrypt.h>

typedef BOOL (WINAPI *CRYPTACQUIRECONTEXTA)(HCRYPTPROV *phProv, LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
typedef BOOL (WINAPI *CRYPTGENRANDOM)(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer);
typedef BOOL (WINAPI *CRYPTRELEASECONTEXT)(HCRYPTPROV hProv, DWORD dwFlags);

static int seed_from_windows_cryptoapi(uint32_t *seed)
{
    HINSTANCE hAdvAPI32 = NULL;
    CRYPTACQUIRECONTEXTA pCryptAcquireContext = NULL;
    CRYPTGENRANDOM pCryptGenRandom = NULL;
    CRYPTRELEASECONTEXT pCryptReleaseContext = NULL;
    HCRYPTPROV hCryptProv = 0;
    BYTE data[sizeof(uint32_t)];
    int ok;

    hAdvAPI32 = GetModuleHandle(TEXT("advapi32.dll"));
    if(hAdvAPI32 == NULL)
        return 1;

    pCryptAcquireContext = (CRYPTACQUIRECONTEXTA)GetProcAddress(hAdvAPI32, "CryptAcquireContextA");
    if (!pCryptAcquireContext)
        return 1;

    pCryptGenRandom = (CRYPTGENRANDOM)GetProcAddress(hAdvAPI32, "CryptGenRandom");
    if (!pCryptGenRandom)
        return 1;

    pCryptReleaseContext = (CRYPTRELEASECONTEXT)GetProcAddress(hAdvAPI32, "CryptReleaseContext");
    if (!pCryptReleaseContext)
        return 1;

    if (!pCryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return 1;

    ok = pCryptGenRandom(hCryptProv, sizeof(uint32_t), data);
    pCryptReleaseContext(hCryptProv, 0);

    if (!ok)
        return 1;

    *seed = buf_to_uint32((char *)data);
    return 0;
}
#endif

/* gettimeofday() and getpid() */
static int seed_from_timestamp_and_pid(uint32_t *seed) {
#ifdef HAVE_GETTIMEOFDAY
    /* XOR of seconds and microseconds */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *seed = (uint32_t)tv.tv_sec ^ (uint32_t)tv.tv_usec;
#else
    /* Seconds only */
    *seed = (uint32_t)time(NULL);
#endif

    /* XOR with PID for more randomness */
#if defined(_WIN32)
    *seed ^= (uint32_t)GetCurrentProcessId();
#elif defined(HAVE_GETPID)
    *seed ^= (uint32_t)getpid();
#endif

    return 0;
}

static uint32_t generate_seed() {
    uint32_t seed;
    int done = 0;

#if !defined(_WIN32) && defined(USE_URANDOM)
    if (!done && seed_from_urandom(&seed) == 0)
        done = 1;
#endif

#if defined(_WIN32) && defined(USE_WINDOWS_CRYPTOAPI)
    if (!done && seed_from_windows_cryptoapi(&seed) == 0)
        done = 1;
#endif

    if (!done) {
        /* Fall back to timestamp and PID if no better randomness is
           available */
        seed_from_timestamp_and_pid(&seed);
    }

    /* Make sure the seed is never zero */
    if (seed == 0)
        seed = 1;

    return seed;
}


volatile uint32_t hashtable_seed = 0;

static volatile char seed_initialized = 0;

void json_object_seed(size_t seed) {
    uint32_t new_seed = (uint32_t)seed;

    if (hashtable_seed == 0) {
        if (new_seed == 0) {
            /* Explicit synchronization fences are not supported by the
               __sync builtins, so every thread getting here has to
               generate the seed value.
            */
            new_seed = generate_seed();
        }

        do {
            if (__sync_bool_compare_and_swap(&hashtable_seed, 0, new_seed)) {
                /* We were the first to seed */
                break;
            } else {
                /* Wait for another thread to do the seeding */
                sched_yield();
            }
        } while(hashtable_seed == 0);
    }
}

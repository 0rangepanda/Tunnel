#include "aes.h"

AesClass::AesClass()
{

}


/**************************************************************************
* Fill in the 128-bit binary key with some text
* better would be to compute the sha1 has of the text,
* but this is OK for a class project.
**************************************************************************/
char* AesClass::gen_key(const char *key_text)
{
        char* key_data = (char*)malloc(sizeof(char)*AES_KEY_LENGTH_IN_CHARS);
        memset(key_data, 0, AES_KEY_LENGTH_IN_CHARS);
        strncpy(key_data, key_text, MIN(strlen(key_text), AES_KEY_LENGTH_IN_CHARS));
        return key_data;
}


/**************************************************************************
* Set encrpyt key
**************************************************************************/
int AesClass::set_encrypt_key(const char *key_text)
{
        AES_set_encrypt_key((unsigned char*)key_text, AES_KEY_LENGTH_IN_BITS, &enc_key);
        return 1;
}


/**************************************************************************
* Set decrypt key
**************************************************************************/
int AesClass::set_decrypt_key(const char *key_text)
{
        AES_set_decrypt_key((unsigned char*)key_text, AES_KEY_LENGTH_IN_BITS, &dec_key);
        return 1;
}


/**************************************************************************
* class_AES_encrypt_with_padding:
* encrypt IN of LEN bytes
* into a newly malloc'ed buffer
* that is returned in OUT of OUT_LEN bytes long
* using ENC_KEY.
*
* It is the *caller*'s job to free(out).
* In and out lengths will always be different because of manditory padding.
**************************************************************************/
int AesClass::encrpyt(unsigned char *in, int len, unsigned char **out, int *out_len)
{
        /*
         * Don't use a 0 IV in the real world,
         * see http://en.wikipedia.org/wiki/Initialization_vector for why.
         * Fortunately class projects are not the real world.
         */
        unsigned char ivec[AES_KEY_LENGTH_IN_BITS/8];
        memset(ivec, 0, sizeof(ivec));

        /*
         * AES requires iput to be an exact multiple of block size
         * (or it doesn't work).
         * Here we implement standard pading as defined in PKCS#5
         * and as described in
         * <http://marc.info/?l=openssl-users&m=122919878204439>
         * by Dave Stoddard.
         */
        int padding_required = AES_KEY_LENGTH_IN_CHARS - len % AES_KEY_LENGTH_IN_CHARS;

        if (padding_required == 0) /* always must pad */
                padding_required += AES_KEY_LENGTH_IN_CHARS;
        assert(padding_required > 0 && padding_required <= AES_KEY_LENGTH_IN_CHARS);
        int padded_len = len + padding_required;
        unsigned char *padded_in = (unsigned char *)malloc(padded_len);
        assert(padded_in != NULL);
        memcpy(padded_in, in, len);
        memset(padded_in + len, 0, padded_len - len);
        padded_in[padded_len-1] = padding_required;

        *out = (unsigned char *)malloc(padded_len);
        assert(*out); /* or out of memory */
        *out_len = padded_len;


        /* finally do it */
        AES_cbc_encrypt(padded_in, *out, padded_len, &enc_key, ivec, AES_ENCRYPT);
        return 1;
}


/**************************************************************************
* class_AES_decrypt:
* decrypt IN of LEN bytes
* into a newly malloc'ed buffer
* that is returned in OUT of OUT_LEN bytes long
* using DEC_KEY.
*
* It is the *caller*'s job to free(out).
* In and out lengths will always be different because of manditory padding.
**************************************************************************/
int AesClass::decrypt(unsigned char *in, int len, unsigned char **out, int *out_len)
{
        unsigned char ivec[AES_KEY_LENGTH_IN_BITS/8];
        /*
         * Don't use a 0 IV in the real world,
         * see http://en.wikipedia.org/wiki/Initialization_vector for why.
         * Fortunately class projects are not the real world.
         */
        memset(ivec, 0, sizeof(ivec));
        *out = (unsigned char *)malloc(len);
        assert(*out);

        AES_cbc_encrypt(in, *out, len, &dec_key, ivec, AES_DECRYPT);

        /*
         * Now undo padding.
         */
        int padding_used = (int)(*out)[len-1];
        //assert(padding_used > 0 && padding_used <= AES_KEY_LENGTH_IN_CHARS); /* or corrupted data */
        *out_len = len - padding_used;
        return 1;
}

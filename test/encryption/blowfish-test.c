/* from http://www.eecis.udel.edu/~bmiller/cis364/2012s/hw/hw3/code/blowfish-test.c, without permission */
/* blowfish-test.c
   Ben Miller
   Small sample for how to use Blowfish from OpenSSL

   compile like so on Solaris:
   cc -g -I/usr/local/openssl/include blowfish-test.c -L/usr/local/openssl/lib -R/usr/local/openssl/lib -lcrypto

   On linux compile like this:
   gcc -g -I/usr/local/openssl/include blowfish-test.c -L/usr/local/openssl/lib  -lcrypto
   *** On mlb add -m32 option to gcc to compile a 32 bit binary as the
   libraries are 32 bit, but gcc defaults to 64 bit. ***

   For details on the Blowfish functions see the man page:
   man -M/usr/local/openssl/man blowfish
*/

#include <stdlib.h>
#include <stdio.h>
#include <openssl/blowfish.h>

#define SIZE 8

int main()
{
  unsigned char *in = (unsigned char *)"TestData";
  unsigned char *out = calloc(SIZE+1, sizeof(char));
  unsigned char *out2 = calloc(SIZE+1, sizeof(char));
  BF_KEY *key = calloc(1, sizeof(BF_KEY));

  /* set up a test key */
  BF_set_key(key, SIZE, (const unsigned char*)"TestKey!" );

  /* test out encryption */
  BF_ecb_encrypt(in, out, key, BF_ENCRYPT);
  printf("%s\n", out);

  /* test out decryption */
  BF_ecb_encrypt(out, out2, key, BF_DECRYPT);
  printf("%s\n", out2);

  return 0;
}

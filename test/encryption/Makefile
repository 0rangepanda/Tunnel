all: blowfish-test aes-test

blowfish-test: blowfish-test.c
	gcc -o $@ -g blowfish-test.c -lcrypto

aes-test: aes-test.c
	gcc -o $@ -g aes-test.c -lcrypto

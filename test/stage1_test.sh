#!/bin/sh
srcdir=`pwd`

cd ../code/src
make proja

echo '\n\n\n\n'

# test cases
./proja
./proja $srcdir/conf

echo '\n\n\n\n'
make clean

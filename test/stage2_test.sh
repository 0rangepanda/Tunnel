#!/bin/sh
srcdir=`pwd`


echo '\n\n\n\n'

#create a tunnel and configure it.
sudo ip tuntap add dev tun1 mode tun
sudo ifconfig tun1 10.5.51.2/24 up

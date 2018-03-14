#!/bin/sh

IP_OF_ETH0=10.0.2.15

sudo ip tuntap add dev tun1 mode tun
sudo ifconfig tun1 10.5.51.2/24 up

ip rule add from $IP_OF_ETH0 table 9 priority 8
ip route add table 9 to 18/8 dev tun1
ip route add table 9 to 128/8 dev tun1
ip route add table 9 to 23/8 dev tun1

iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP

sudo ifconfig eth1 192.168.201.2/24 up
sudo ifconfig eth2 192.168.202.2/24 up
sudo ifconfig eth3 192.168.203.2/24 up
sudo ifconfig eth4 192.168.204.2/24 up
sudo ifconfig eth5 192.168.205.2/24 up
sudo ifconfig eth6 192.168.206.2/24 up

# Tor Project
## Stage 1: Starting Process
This step create an Onion Proxy program that is responsible for forking an Onion Router. The Proxy will hook in to the networking system through a tunnel network interface that lets it scoop up network traffic.

The Proxy first create a dynamic (operating-system assigned) UDP port using *getsockname()*. Then the Proxy pass its dynamically allocated UDP port process to the child through a global variable. Next, the Proxy will then wait for the Router to send it an “I’m up” message on its UDP port.

When the Router starts, it gets its process id and send an “I’m up” message (Just send router’s PID) to the Proxy’s UDP socket.

There are also log system and ctrl+c signal catching for ending the program decently.

## Stage 2: Sending Traffic
Open a tunnel
```python
> sudo ip tuntap add dev tun1 mode tun
> sudo ifconfig tun1 10.5.51.2/24 up
```

The first command creates a new tunnel interface named tun1. The second command sets the IP address of the other end of the tunnel to 10.5.51.2, and brings it up.

After doing these steps, all the traffic sent to the 10.5.51/24 address block will go into tun1. So if you ping address 10.5.51.x from a new terminal, all the ICMP packets will go into tun1.

In this step, router and proxy should be able to relay the traffic as following:

>**ping 10.5.51.x: icmp -> tun1 -> proxy -> router**

>**router: icmp reply -> proxy -> ping**



## Stage 3: Pinging the Real World
### 3.1 Setting up traffic to your proxy and out of your VM

Open a tunnel
```c
> sudo ip tuntap add dev tun1 mode tun
> sudo ifconfig tun1 10.5.51.2/24 up
```


Redirect some traffic to our tunnel interface
```c
> sudo ip rule add from $IP_OF_ETH0 table 9 priority 8
> sudo ip route add table 9 to 18/8 dev tun1
> sudo ip route add table 9 to 128/8 dev tun1
```

Discard Linux RST packets
```c
> sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP
```
After these commands, ping -I $IP_OF_ETH0 www.csail.mit.edu will go into your proxy.

*For fedora, need to change the interface name to traditional style* [link](http: //esuareznotes.wordpress.com/2014/07/11/)

### 3.2 Creating user-controlled routers that can talk to the Internet

In this step, we allow traffic to go out of your routers to go out of the VM guest OS into the Internet.

First, we need to create additional virtual network interfaces in your virtual machine, then we need to have your routers use them, and finally we need to allow their traffic to go out and replies to come back in.

1. To add more network interfaces, use command line to add up to eight interfaces (of course on the host machine)
```c
> VBoxManage modifyvm [VM_NAME] --nic8 nat
```
Then you need to configure the second ethernet with the following command:
```c
> sudo ifconfig eth1 192.168.201.2/24 up
```
This command both brings up eth1 with IP address 192.168.201.2, and it assigns 192.168.201/24 as a subnet that goes to eth1.

2.

3.











# END

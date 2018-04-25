a) Reused Code:
  - in_cksum() is from the TA's current_projb code
  - in_cksum_tcp() is modified from TA's code. Changed to allow computing checksum of different length of TCP packet.

b) Complete:
  Stage 7 complete

c) What would happen to your TCP connection if the kernel’s RST packets were not filtered out?
  For Tor can lead to very long delay, if not filtered out RST, client may send RST to
  terminate the TCP connection.

d) Why does the Linux kernel generate a RST packet for TCP, while it ignored ICMP and UDP packets in prior stages?
  The RST flag is used to signal any kind of error and terminate the connection.
  - If you send a packet with a wrong ACK, you will get a RST
  - If you don't acknowledge data in a timely manner, you get a RST

  One reason is that TCP is stateful and more costly . ICMP and UDP packets has no
  connection to end. 

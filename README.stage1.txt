a) Reused Code:
  in utils.cpp:
    * tun_alloc: allocates or reconnects to a tun/tap device.
    * copy from from simpletun.c
    * refer to http://backreference.org/2010/03/26/tuntap-interface-tutorial/ for more info


b) Complete:
  Stage 1 compelete

c) Portable:
Will your code always work if the proxy and the router were on different computers with different
CPU architectures, like an IBM PowerPC and an AMD x86-64? If so, why (what specifically did you
do to support that case)? If not, why not (what problem would occur if run between different types
of computers)? (Note that in your Project A, they are always on the same computer architecture
because you’re just forking another process, not connecting to another computer. This question is
about the hypothetical case of if they were on different computers with different CPU architectures.)

For the program itself, as long as no hardware drive and no assembly instruction are involved,
it will always work under the same OS no matter which CPU architecture is used.

a) Reused Code:
  No

b) Complete:
  Stage 3 complete

c) Addressing on the way out of your router:
Why is it important to rewrite the source address in your proxy? (What would happen if you left it as the original address of the VM?)

First we want to hide the src IP from the outside world.
Second, if not rewriting, router will not see the reply message.

d) Addressing on the way in to the VM:
Why is it important that there is a separate network device for your router in the VM?

So that router won't get packet that sended by VM guest.

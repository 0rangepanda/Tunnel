a) Reused Code:
  - No

b) Complete:
  - Yes

c) In this stage we are careful to make sure all packets for a given flow take the same path.
What bad thing might happen if different packets from one flow took different paths
of different lengths?
  - Packet reordering.

d) Continuing (c), suppose all paths were the same length and they were sent slowly (say,
one packet every millisecond), but each packet went over a different circuit. Would
the problem you identified in (c) be likely to occur in our simple test network on one
machine?
  - Not likely on one machine, cause every path has roundly the same latency.

e) Continuing (d), now suppose paths were the same length and packets were sent every
millisecond, but now Mantitor nodes were anywhere in the Internet, not all one test
machine. Now, would the problem you identified in (c) be likely?
  - Yes. For things like congestion and queueing could happen in the Internet,
    latency now can have very big variance.

a) Reused Code:
  No

b) Complete:
  Stage 2 complete

c) Router Selection:
For the current approach to mapping traffic to routers,
i) Why is that approach load balanced (at least statistically) over many flows to many targets?
  Because statistically the value of ip address mod number of routers has a uniform distribution.

ii) Are there degenerate cases where your load could become unbalanced? (Just yes or no.)
  Yes

iii) What is one such case?
  Ping some specific ip address more than the others.

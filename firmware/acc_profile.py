#!/usr/bin/python
from __future__ import division
import math

k = 127./3000.   # mm/halfstep
a = 1e-10

t = [ int(math.sqrt(2*k*i/a)) for i in range(0,200)]

delays = [ t[i+1] - t[i] for i in range(0, len(t)-1) ]

print "/* This file is generated, DO NOT EDIT. */"
print ""
print "const unsigned int n_acc_delays = %d;" % len(delays)
print "const unsigned int acc_delays[] = {"
for i in range(0, len(delays)-1):
    print ("%d," % delays[i]),
    if i % 10 == 9:
        print ""
print "%d" % delays[-1]
print "};"

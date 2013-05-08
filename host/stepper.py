#!/usr/bin/env python
from __future__ import division
from pcbwriter import PCBWriter
import sys

if len(sys.argv) < 2 or sys.argv[1] not in ("a", "r", "h") \
    or (sys.argv[1] == "h" and len(sys.argv) != 2) \
    or (sys.argv[1] != "h" and len(sys.argv) != 3):
    print "Usage: %s a|r|h [<pos>]" % sys.argv[0]
    sys.exit(1)

pcb = PCBWriter()

print "Absolute position before move: %d%s" % (pcb.get_stepper_status().pos,
    " [HOMED]" if pcb.get_stepper_homed() else "")

if sys.argv[1] == "h":
    pcb.home_stepper(wait=True)
elif sys.argv[1] == "a":
    pcb.move_stepper(int(sys.argv[2]), relative=False, wait=True)
elif sys.argv[1] == "r":
    pcb.move_stepper(int(sys.argv[2]), relative=True, wait=True)

print "Absolute position after move: %d%s" % (pcb.get_stepper_status().pos,
    " [HOMED]" if pcb.get_stepper_homed() else "")

#!/usr/bin/env python
from __future__ import division
import math
import array
import subprocess
import usb.core
import usb.util
import usb.control

dev = usb.core.find(idVendor = 0x1337, idProduct = 0xabcd)

if dev is None:
    raise RuntimeError("Device not found")

print "Getting flash:"
ret = dev.ctrl_transfer(bmRequestType=0xC0, bRequest=0x83, wValue=64, wIndex=0, data_or_wLength=64, timeout=1000)
print ret

print "Setting flash:"
ret = dev.ctrl_transfer(bmRequestType=0x40, bRequest=0x82, wValue=0x21, wIndex=31, data_or_wLength=None, timeout=1000)

print "Getting flash:"
ret = dev.ctrl_transfer(bmRequestType=0xC0, bRequest=0x83, wValue=64, wIndex=0, data_or_wLength=64, timeout=1000)
print ret

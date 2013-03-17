#!/usr/bin/env python
from __future__ import division
import math
import array
import subprocess
import usb.core
import usb.util

def load_image(fname, bbox, scale):
    bbox_width = bbox[2] - bbox[0]
    bbox_height = bbox[3] - bbox[1]
    
    args = ["gs"]
    args += ["-o", "%stdout%"]
    args += ["-dQUIET"]
    args += ["-dLastPage=1"]
    args += ["-sDEVICE=bit"]
    args += ["-r%d" % (scale*72)]
    args += ["-g%dx%d" % (scale*bbox_width, scale*bbox_height)]
    args += ["-c", "<</Install {%d %d translate}>> setpagedevice" % (-bbox[0], -bbox[1])]
    args += ["-f", fname]

    print " ".join(args)

    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout_data, stderr_data) = p.communicate(None)

    if stderr_data != "":
        print "Ghostscript subprocess encountered an error:"
        print stderr_data
        raise RuntimeError
    
    img = array.array("B", stdout_data)
    
    return img

# FIXME
bbox_x0 = 269
bbox_y0 = 269
bbox_x1 = 423
bbox_y1 = 315
scale = 300

bbox_width = bbox_x1 - bbox_x0
bbox_height = bbox_y1 - bbox_y0

bytes_per_line = int(math.ceil(scale*bbox_width / 8.))

print "Loading image:"
img = load_image("test.pdf", (bbox_x0, bbox_y0, bbox_x1, bbox_y1), scale)

print "Transferring image: "
PCBWRITER_TIMEOUT = 100 # ms

dev = usb.core.find(idVendor = 0x1337, idProduct = 0xabcd)
if dev is None:
    raise RuntimeError("Device not found")

for line in range(0, bbox_height*scale):
    print "Transferring line %d/%d" % (line, bbox_height*scale)
    if dev.write(1, img[line*bytes_per_line:(line+1)*bytes_per_line], 0, PCBWRITER_TIMEOUT) != bytes_per_line:
        raise RuntimeError, "Failed to communicate with endpoint."
    if dev.write(1, array.array("B", [0]*(6000-bytes_per_line)).tostring(), 0, PCBWRITER_TIMEOUT) != (6000 - bytes_per_line):
        raise RuntimeError, "Failed to communicate with endpoint."


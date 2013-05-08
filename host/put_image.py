#!/usr/bin/env python
from __future__ import division
import math
from array import array
import subprocess
import time
import sys
import ghostscript
import argparse
from pcbwriter import PCBWriter

parser = argparse.ArgumentParser(description="Send data to PCBWriter device.")
parser.add_argument("-b", "--bbox", help="specify bounding box")
# parser.add_argument("-t", "--top-margin", help="top margin (mm)")
# parser.add_argument("-l", "--left-margin", help="left margin (mm)")
parser.add_argument("fname")

args = parser.parse_args()

def pos(px):
    a = 3.24776e-13
    b = -2.62591e-08
    c = 0.00277007
    d = 0.417002
    
    return a*px**3 + b*px**2 + c*px + d

def sample(line, pos):
    px = int(math.ceil(pos * 100. - 0.5))
    if line[px // 8] & (128 >> (px % 8)):
        return 1
    else:
        return 0

def transform_line(line):
    output = array("B", [0]*6000)
    
    for px in range(0, 48000):
        if not sample(line, pos(px)):
            output[px // 8] = output[px // 8] | (128 >> (px % 8))
    
    return output

bbox = None
if args.bbox:
    try:
        bbox = [ float(s) for s in args.bbox.split(",")]
        if not len(bbox) == 4:
            raise ValueError
    except ValueError:
        print "Bounding box format: <x0>,<y0>,<x1>,<y1>"
        sys.exit(1)

if bbox == None:
    bbox = ghostscript.get_bbox(args.fname)

left_margin = 10.
right_margin = 0.
top_margin = 0.
bottom_margin = 0.

bbox[0] -= (left_margin/25.4*72.0)
bbox[1] -= (top_margin/25.4*72.0)
bbox[2] += (right_margin/25.4*72.0)
bbox[3] += (bottom_margin/25.4*72.0)

xres = 2540   # dpi, i.e 10um per pixel
yres = 600

bbox_width = bbox[2] - bbox[0]
bbox_height = bbox[3] - bbox[1]

width_px = 11000 # Render 110 mm strip
height_px = int(math.ceil((yres*bbox_height)/72))

bytes_per_line = int(math.ceil(width_px / 8))

print "Loading image (%dx%d pixels):" % (width_px, height_px)
img = ghostscript.load_image(args.fname, bbox, xres, yres, width_px, height_px)

print "Setting up device"
pcb = PCBWriter()

pcb.set_n_scans(84)
pcb.set_autostep(True)

print "%d bytes/line" % bytes_per_line

print "Transferring image: "

for line in range(0, height_px):
    print "Transferring line %d/%d" % (line, height_px)
    
    # line_data = array.array("B", [255]*2000)
    line_data = transform_line(img[line*bytes_per_line:(line+1)*bytes_per_line])
    
    pcb.put_line(line_data, fill=False, wait=True)


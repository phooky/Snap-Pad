#!/usr/bin/python
import sys
import nandlib
import time

n = nandlib.Nand('/dev/ttyACM0')

spare_start = 2048

block_count = 1024
plane_count = 2
page_count = 64

def make_addr(plane,block,page,column=0):
    addr = (column << 0) | (page << 12) | (plane << 18) | (block << 19)
    return addr

def check_bad(plane,block):
    pages = [0,1,page_count-1]
    for page in pages:
        a = make_addr(plane,block,page,spare_start)
        b = ord(n.nandRead(a,1)[0])
        if b != 0xff:
            print 'Bad block: {0}:{1} read: {2:02X}'.format(plane,block,b)
            return True
    return False


print "Begin scan..."
for plane in range(plane_count):
    for block in range(block_count):
        check_bad(plane,block)
print "scan complete."

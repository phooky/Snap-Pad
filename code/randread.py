#!/usr/bin/python
import serial
import sys

p = serial.Serial('/dev/ttyACM0',115200)
#p = sys.stdout

def read():
    c = '#\n\r'
    p.write(c)
    p.flush()
    return p.read(16*4)

def dump(l):
    while l > 255 or l < 0:
        sys.stdout.write(read())
        l = l - 255
    sys.stdout.write(read())

if __name__ == '__main__':
    dump(-1)
             

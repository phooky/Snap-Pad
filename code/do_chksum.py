#!/usr/bin/python
import serial
import sys
import argparse
import os

p = None
#p = sys.stdout

def check(block):
    global p
    c = 'c{0}\n\r'.format(block)
    p.write(c)
    p.flush()
    p.readline()
    print "block {0} {1}".format(block,p.readline().split()[-1])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Checksum a set of blocks off of a snap-pad')
    parser.add_argument('-c', '--count', type=int, default=2047)
    parser.add_argument('-s', '--start', type=int, default=1)
    parser.add_argument('-p', '--prefix', type=str, default="SP-NAND")
    parser.add_argument('-P', '--port', default='/dev/ttyACM0')
    args = parser.parse_args()
    p = serial.Serial(args.port,115200)
    p.flushInput()
    p.flush()
    for block in range(args.start,args.start+args.count):
        check(block)


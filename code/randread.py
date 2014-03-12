#!/usr/bin/python
import serial
import sys
import argparse

p = None
#p = sys.stdout

def read():
    global p
    c = 'P\n\r'
    p.write(c)
    p.flush()
    return p.read(512)

def dump(l):
    while l > 512 or l < 0:
        sys.stdout.write(read())
        l = l - 512
    if l > 0:
        sys.stdout.write(read()[0:l])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Pull random numbers from the snap-pad.')
    parser.add_argument('-c', '--count', type=int, default=-1)
    parser.add_argument('-P', '--port', default='/dev/ttyACM0')
    args = parser.parse_args()
    p = serial.Serial(args.port,115200)
    dump(args.count)
             

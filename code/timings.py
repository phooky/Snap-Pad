#!/usr/bin/python
import serial
import sys
import argparse
import time

p = None
#p = sys.stdout

def read(command):
    global p
    c = command + '\n\r'
    p.write(c)
    p.flush()
    l = p.readline().strip()
    print l
    t = time.time()
    l = p.readline().strip()
    print l
    return time.time()-t


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Pull random numbers from the snap-pad.')
    parser.add_argument('-P', '--port', default='/dev/ttyACM0')
    args = parser.parse_args()
    p = serial.Serial(args.port,115200)
    rt = read("Tr")
    print "4 blocks rand:",rt,"seconds"
    print "Estimated total:",rt*(2048/4),"seconds,",rt*(2048/4)/3600.0,"hours"
    wt = read("Tw")
    print "4 blocks write:",wt,"seconds"
    print "Estimated total:",wt*(2048/4),"seconds,",wt*(2048/4)/3600.0,"hours"
    

             

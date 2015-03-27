#!/usr/bin/python3

import serial
import argparse
from sys import stdout, stderr
import os.path
import os
import time

class Pad:
    def __init__(self,portname):
        self.port = serial.Serial(portname)
        self.port.flushOutput()
        self.port.flushInput()
        time.sleep(0.5)
        self.port.flush()
    def get(self,block,page,para):
        self.port.flushInput()
        self.port.flush()
        self.port.write(('r{0},{1},{2}\n').format(block,page,para).encode())
        self.port.flush()
        return self.port.read(512)

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Retrieve all data from debug mode snap-pad.')
    parser.add_argument('port',type=str,help='serial port for device',default='/dev/ttyACM0')
    parser.add_argument('path',type=str,help='path to file')
    args=parser.parse_args()
    pad = Pad(args.port)
    path = args.path
    firstblock = 1
    if os.path.exists(path):
        if not os.path.isfile(path):
            raise ValueError("{0} is a not an ordinary file".format(path))
        sz = os.stat(path).st_size
        paras = int(sz / 512)
        pages = int(paras / 4)
        blocks = int(pages / 64)
        firstblock = 1 + blocks
        f = open(path,'r+b')
        f.truncate(blocks*64*4*512)
        f.seek(blocks*64*4*512)
        stdout.write("Continuing {0} at block {1}.\n".format(path,firstblock))
    else:
        f = open(path,'wb')
        stdout.write("New file {0}.\n".format(path))
    for block in range(firstblock,2048):
        stderr.write("Reading block {0:04}".format(block))
        stderr.flush()
        for page in range(64):
            stderr.write(".")
            stderr.flush()
            for para in range(4):
               f.write(pad.get(block,page,para))
        stderr.write("done.\n")
        stderr.flush()
    f.close()

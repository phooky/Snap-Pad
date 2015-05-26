#!/usr/bin/python

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
        self.port.timeout = 0.5
    def get(self,page):
        self.port.write(('r{0}\n').format(page).encode())
        #self.port.flush()
        data = b''
        while len(data) < 2048:
            part = self.port.read(2048 - len(data))
            if len(part) == 0:
                break
            data = data + part
        if len(data) < 2048:
            raise Exception('Could not read page {0}: size {1}\n {2}\n'.format(page, len(data), data[-1]))
        return data

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Retrieve all data from debug mode snap-pad.')
    parser.add_argument('--port',type=str,help='serial port for device',default='/dev/ttyACM0')
    parser.add_argument('path',type=str,help='path to file')
    parser.add_argument('--first',type=int,default=64,help='first page')
    parser.add_argument('--count',type=int,default=(4095*64),help='number of pages')
    args=parser.parse_args()
    pad = Pad(args.port)
    path = args.path
    firstpage = args.first
    if os.path.exists(path):
        if not os.path.isfile(path):
            raise ValueError("{0} is a not an ordinary file".format(path))
        sz = os.stat(path).st_size
        pages = int(sz / 2048)
        firstpage = firstpage + pages
        f = open(path,'r+b')
        f.truncate(pages*2048)
        f.seek(pages*2048)
        stdout.write("Continuing {0} at page {1}.\n".format(path,firstpage))
    else:
        f = open(path,'wb')
        stdout.write("New file {0}.\n".format(path))
    for page in range(firstpage,firstpage+args.count):
        stderr.write("Reading page {0:06}i...".format(page))
        stderr.flush()
        f.write(pad.get(page))
        stderr.write("done.\n")
        stderr.flush()
    f.close()


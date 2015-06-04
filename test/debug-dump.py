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
        self.port.timeout = 0.5
        self.top = (2048 * 64) # top of page space
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
            raise Exception('Could not read page {0}: size {1}\n {2}\n'.format(page, len(data), data))
        return data

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Retrieve all data from debug mode snap-pad.')
    parser.add_argument('--port',type=str,help='serial port for device',default='/dev/ttyACM0')
    parser.add_argument('path',type=str,help='path to file',default='-')
    parser.add_argument('--first',type=int,default=64,help='first page')
    parser.add_argument('--count',type=int,default=-1,help='number of pages')
    args=parser.parse_args()
    pad = Pad(args.port)
    path = args.path
    firstpage = args.first
    if args.count == -1:
        count = pad.top - firstpage
    else:
        count = args.count
    if os.path.exists(path):
        if not os.path.isfile(path):
            raise ValueError("{0} is a not an ordinary file".format(path))
        sz = os.stat(path).st_size
        pages_so_far = int(sz / 2048)
        firstpage = firstpage + pages_so_far
        f = open(path,'r+b')
        f.truncate(pages_so_far*2048)
        f.seek(pages_so_far*2048)
        counnt = count - pages_so_far
        stderr.write("Continuing {0} at page {1}.\n".format(path,firstpage))
    else:
        if path == '-':
            f = stdout.buffer
        else:
            f = open(path,'wb')
            stderr.write("New file {0}.\n".format(path))
    stderr.write("Retrieving pages {0} to {1}".format(firstpage,firstpage+count))
    for page in range(firstpage,firstpage+count):
        stderr.write("Reading page {0:06}...".format(page))
        stderr.flush()
        f.write(pad.get(page))
        stderr.write("done.\n")
        stderr.flush()
    f.close()


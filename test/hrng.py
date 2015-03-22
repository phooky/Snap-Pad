#!/usr/bin/python3

import serial
import argparse
from sys import stdout

class Pad:
    def __init__(self,portname):
        self.port = serial.Serial(portname)
    def get16(self):
        self.port.flushInput()
        self.port.flush()
        self.port.write(('#\n').encode())
        return self.port.read(16)

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Retrieve random data from Snap-Pad.')
    parser.add_argument('port',type=str,help='serial port for device',default='/dev/ttyACM0')
    parser.add_argument('bytes',type=int,help='number of bytes to retrieve',default=2048)
    args=parser.parse_args()
    pad = Pad(args.port)
    bc = args.bytes
    forever = False
    if bc == -1:
        forever = True
    while forever or bc > 0:
        d = pad.get16()
        if not forever:
            if bc<16:
                d = d[:bc]
            bc = bc - 16
        stdout.buffer.write(d)



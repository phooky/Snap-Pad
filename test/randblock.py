#!/usr/bin/python3

import serial
import argparse
from sys import stdout

class Pad:
    def __init__(self,portname):
        self.port = serial.Serial(portname)

    def getBlock(self):
        self.port.flushInput()
        self.port.flush()
        self.port.write(('R\n').encode())
        d = b''
        for i in range(64 * 4 * 512):
            b = int(self.port.read(2),16)
            d = d + bytes([b])
        return d

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Retrieve random data from Snap-Pad.')
    parser.add_argument('port',type=str,help='serial port for device',default='/dev/ttyACM0')
    args=parser.parse_args()
    pad = Pad(args.port)
    stdout.buffer.write(pad.getBlock())



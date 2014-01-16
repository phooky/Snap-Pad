#!/usr/bin/python
import serial
import sys
import time

class Nand:
    def __init__(self,port='/dev/ttyACM0'):
        self.p = serial.Serial(port,9600,timeout=0.01)

    def cmd(self,val):
        c = 'HC{0:02X}\n'.format(val)
        self.p.write(c)
        self.p.flush()

    def addr(self,val):
        c = 'HA{0:02X}\n'.format(val)
        self.p.write(c)
        self.p.flush()

    def read(self,val):
        c = 'HR{0:02X}\n'.format(val)
        self.p.write(c)
        self.p.flush()
        v = self.p.read(val)
        return v

    def enterOTP(self):
        self.cmd(0x29)
        self.cmd(0x17)
        self.cmd(0x04)
        self.cmd(0x19)

    def nandRead(self,addr,l):
        self.p.write('R:{0:04X}:{1:04X}\n'.format(addr,l))
        return self.p.read(l)

    def dump(self,l,out=sys.stdout):
        while l > 16 or l < 0:
            out.write(self.read(16))
            l = l - 16
        out.write(self.read(l))

             

#!/usr/bin/python
import serial
import sys

p = serial.Serial('/dev/ttyACM0',115200)

def cmd(val):
    c = 'HC{0:02X}'.format(val)
    p.write(c)

def addr(val):
    c = 'HA{0:02X}'.format(val)
    p.write(c)

def read(val):
    c = 'HR{0:02X}'.format(val)
    p.write(c)
    return p.read(val)

def dump(l):
    while l > 256 or l < 0:
        sys.stdout.write(read(256))
        l = l - 256
    sys.stdout.write(read(l))

def h_id(length=-1):
    p.cmd(0x90)
    p.addr(0x00)
    dump(length)

def h_id2(length=-1):
    p.cmd(0x30)
    p.cmd(0x65)
    p.addr(0x00)
    p.addr(0x02)
    p.addr(0x02)
    p.addr(0x00)
    p.addr(0x00)
    p.addr(0x00)
    p.cmd(0x30)
    dump(length)

def h_onfi(length=-1):
    p.cmd(0x90)
    p.addr(0x20)
    dump(length)

if __name__ == '__main__':
    if len(sys.argv) == 0:
        h_id(2048)
        h_id2(2048)
        h_onfi(2048)
    elif argv[1] == 'ID':
        h_id()
    elif argv[1] == 'ID2':
        h_id2()
    elif argv[1] == 'ONFI':
        h_onfi()

             

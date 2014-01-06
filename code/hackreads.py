#!/usr/bin/python
import serial
import sys

p = serial.Serial('/dev/ttyACM0',115200)
#p = sys.stdout
def cmd(val):
    c = 'HC{0:02X}\n\r'.format(val)
    p.write(c)
    p.flush()

def addr(val):
    c = 'HA{0:02X}\n\r'.format(val)
    p.write(c)
    p.flush()

def read(val):
    c = 'HR{0:02X}\n\r'.format(val)
    p.write(c)
    p.flush()
    return p.read(val)

def dump(l):
    while l > 255 or l < 0:
        sys.stdout.write(read(255))
        l = l - 255
    sys.stdout.write(read(l))

def h_id(length=-1):
    cmd(0x90)
    addr(0x00)
    dump(length)

def h_id2(length=-1):
    cmd(0x30)
    cmd(0x65)
    addr(0x00)
    addr(0x02)
    addr(0x02)
    addr(0x00)
    addr(0x00)
    addr(0x00)
    cmd(0x30)
    dump(length)

def h_onfi(length=-1):
    cmd(0x90)
    addr(0x20)
    dump(length)

if __name__ == '__main__':
    if len(sys.argv) == 0:
        h_id(2048)
        h_id2(2048)
        h_onfi(2048)
    elif sys.argv[1] == 'ID':
        h_id()
    elif sys.argv[1] == 'ID2':
        h_id2()
    elif sys.argv[1] == 'ONFI':
        h_onfi()

             

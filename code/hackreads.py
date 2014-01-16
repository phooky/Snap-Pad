#!/usr/bin/python
import serial
import sys
import nandlib
import time

n = nandlib.Nand('/dev/ttyACM0')

def h_id(length=-1):
    n.cmd(0x90)
    n.addr(0x00)
    n.dump(length)

def h_id2(length=-1):
    n.cmd(0x30)
    n.cmd(0x65)
    n.addr(0x00)
    n.addr(0x02)
    n.addr(0x02)
    n.addr(0x00)
    n.addr(0x00)
    n.addr(0x00)
    n.cmd(0x30)
    n.dump(length)

def h_onfi(length=-1):
    n.cmd(0x90)
    n.addr(0x20)
    n.dump(length)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        h_id(2048)
        h_id2(2048)
        h_onfi(2048)
    elif sys.argv[1] == 'ID':
        h_id()
    elif sys.argv[1] == 'READ':
        sys.stdout.write(n.nandRead(int(sys.argv[2]),int(sys.argv[3])))
    elif sys.argv[1] == 'OTP':
        n.enterOTP()
        l = ''
        for i in range(0,2048*4):
            ln = n.nandRead(i*16,16)
            if ln != l:
                sys.stdout.write('{')
                sys.stdout.write(ln)
                sys.stdout.write('}\n')
                l = ln
    elif sys.argv[1] == 'INFO':
        #n.p.write('HC90\n')
        #n.p.write('HHA20\n')
        n.p.flush()
        time.sleep(0.1)
        for i in range(0,2048*4):
            n.p.write('I\n')
            print n.p.read(100)
            n.p.write('HC90\n')
            n.p.write('HA20\n')
            n.p.write('HR04\n')
            print n.p.read(4)
    elif sys.argv[1] == 'SCAN':
        for addr in range(0,0x200):
            n.cmd(0x90)
            n.addr(addr)
            sys.stdout.write('\n@{0:04X} '.format(addr))
            n.dump(80)
    elif sys.argv[1] == 'ID2':
        h_id2()
    elif sys.argv[1] == 'ONFI':
        h_onfi()

             

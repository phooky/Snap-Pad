#!/usr/bin/python3

import serial
import argparse
import select
from sys import stdin

class PadTest:
    def set_port(self,port):
        self.port = port
    def run_command(self,command,timeout=5,rawb=0):
        self.port.timeout = timeout
        self.port.flushInput()
        self.port.flush()
        self.port.write((command+'\n').encode())
        if rawb > 0:
            self.raw = self.port.read(rawb)
            self.rsp = self.raw
            dat = b''
        #print(self.port.readline())
        else:
            dat = self.port.readline().strip()
            self.rsp = dat
        return dat
    def instructions(self,message):
        "Instructions for interactive tests"
        print(message)
    def query(self,message):
        "Ask a question of the user"
        print(message)
        self.rsp=stdin.readline().strip().encode()
    def q_timeout(self,timeout):
        "Check for a response w/ given timeout"
        rfds,_,_ = select.select( [stdin], [], [], timeout)
        if len(rfds) > 0:
            self.rsp = stdin.readline().strip().encode()
            return True
        else:
            return False 
    def expect(self,value):
        if value.encode() == self.rsp:
            self.ok(value)
        else:
            self.error('Expected {0}, got {1}'.format(value,self.rsp))
    def ok(self,message=''):
        self.result=True
        self.message=message
    def error(self,message=''):
        self.result=False
        self.message=message

class VersionTest(PadTest):
    def __init__(self,version='1.1'):
        self.expected_version=version
        self.name='Firmware version'
    def run_test(self):
        version = self.run_command('V') 
        self.expect(self.expected_version)

class PingTest(PadTest):
    def __init__(self):
        self.name='UART test'
    def run_test(self):
        self.run_command('P')
        self.expect('OK')

class ONFITest(PadTest):
    def __init__(self):
        self.name='NAND ONFI test'
    def run_test(self):
        self.run_command('NO')
        self.expect('OK')

class ButtonInteractiveTest(PadTest):
    def __init__(self):
        self.name='Button test (interactive)'
    def run_test(self):
        self.instructions('Press the button exactly twice within the next ten seconds')
        self.run_command('B',timeout=10.5)
        self.expect('2')

class LEDAttractTest(PadTest):
    def __init__(self):
        self.name='LED test (interactive)'
    def run_test(self):
        self.instructions('Are the LEDs couting up? (y/n)')
        ledv = 0
        while not self.q_timeout(0.1):
            self.run_command('L{0:x}'.format(ledv))
            ledv = (ledv+1)%16
        self.expect('y')

class LEDInteractiveTest(PadTest):
    def __init__(self,pattern):
        self.name='LED test (interactive)'
        self.pattern = pattern
    def run_test(self):
        self.run_command('L{0:x}'.format(self.pattern))
        self.query('Are the LEDs displaying pattern {0:b}? (y/n)'.format(self.pattern))
        self.expect('y')

class RandomTest(PadTest):
    def __init__(self,bytecount):
        self.name='HRNG test'
        self.bytecount=bytecount
    def run_test(self):
        bc = self.bytecount
        data = b''
        while bc > 0:
            bc = bc - 16
            self.run_command('#',rawb=16)
            if self.rsp != 'OK'.encode():
                self.error('Error reading raw data')
            d = self.raw
            for b in d:
                print('{:02x}'.format(b),end='')
            data = data + d
            print()
        self.ok('Random data acquired')

all_tests = [
        VersionTest(), PingTest(), ONFITest(),
        #LEDAttractTest(),
        #ButtonInteractiveTest(),
        RandomTest(256),
        #LEDInteractiveTest(0x5),
        #LEDInteractiveTest(0xa)
        ]

def test_pad(portname):
    pad = serial.Serial(portname)
    for test in all_tests:
        test.set_port(pad)
        test.run_test()
        print(test.name,test.result,test.message)
    pad.close()

if __name__=='__main__':
    parser = argparse.ArgumentParser(description='Factory test for Snap-Pad.')
    parser.add_argument('port',type=str,help='serial port for device',default='/dev/ttyACM0')
    args=parser.parse_args()
    test_pad(args.port)


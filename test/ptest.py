#!/usr/bin/python3

import serial
import argparse

class PadTest:
    def set_port(self,port):
        self.port = port
    def run_command(self,command,timeout=5,rawb=0):
        self.port.timeout = timeout
        self.port.flushInput()
        self.port.flush()
        self.port.write((command+'\n').encode())
        dat = b''
        if rawb > 0:
            dat = dat + self.port.read(rawb)
        #print(self.port.readline())
        dat = dat + self.port.readline().strip()
        self.rsp = dat
        return dat
    def expect(self,value):
        if value.encode() == self.rsp:
            self.ok(value)
        else:
            self.error('Expected {0}, got {1}'.format(value,self.rsp.encode()))
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

all_tests = [
        VersionTest(), PingTest(), ONFITest()
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


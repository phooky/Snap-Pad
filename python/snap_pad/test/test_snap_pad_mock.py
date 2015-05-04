
import sys
import random
import time
import serial
import base64

sys.stderr.write("*** WARNING: You are importing a test module. This is not for production use!\n")

MAJOR = 1
MINOR = 1
VARIANT = 'M'

class SnapPadHWMock(serial.FileLike):
    "A software mockup of a hardware snap-pad."
    def __init__(self,seed=1,timeout=0,major=MAJOR,minor=MINOR,
            variant=VARIANT,mode='Single board',*args,**kwargs):
        self.d = kwargs
        self.rng = random.Random()
        self.rng.seed(seed)
        self.timeout = timeout
        self.major = major 
        self.minor = minor 
        self.variant = variant 
        self.mode = mode 
        self.kwargs = kwargs
        self.diagnostics = { 'Debug':'true', 'Mode':'Single board', 'Random':'Done', 'Blocks':'2047' }
        self.buffer = ''
        self.outbuf = ''

    def do_version(self):
        self.outbuf += '{0}.{1}{2}\n'.format(self.major,self.minor,self.variant)

    def do_diagnostics(self):
        self.outbuf += '---BEGIN DIAGNOSTICS---\n'
        if self.diagnostics:
            for k,v in self.diagnostics.items():
                self.outbuf += '{0}:{1}\n'.format(k,v)
        else:
            self.outbuf += 'Error: Test mockup; no diagnostics.\n'
        self.outbuf += '---END DIAGNOSTICS---\n'

    def release_para(self,block,page,para):
        self.outbuf += '---BEGIN PARA {0},{1},{2}---\n'.format(block,page,para)
        # reseed rng?
        rng = self.rng
        parasz = 512
        o = base64.b64encode(bytearray([rng.randint(0,255) for x in range(parasz)]))
        while o:
            self.outbuf += o[:80] + '\n'
            o = o[80:]
        self.outbuf += '---END PARA---\n'

    def provision_one(self):
        self.release_para(1,1,1)

    def do_error(self,msg):
        self.outbuf += 'ERROR: '
        self.outbuf += msg
        self.outbuf += '\n'

    def do_provision(self,command):
        count = int(command[1:])
        if count < 1 or count > 4:
            self.do_error('bad count')
            return
        for _ in range(count):
            self.provision_one()

    def do_retrieve(self,command):
        spec = command[1:].split(',')
        while len(spec)>0:
            addr, spec = spec[:3], spec[3:]
            self.release_para(addr[0],addr[1],addr[2])

    def do_rng(self):
        self.outbuf += bytearray([self.rng.randint(0,255) for x in range(64)])

    def write(self,data):
        self.buffer = self.buffer + data
        if self.buffer.find('\n') != -1:
            command, self.buffer = self.buffer.split('\n',1)
            if command[0] == 'V':
                self.do_version()
            elif command[0] == 'D':
                self.do_diagnostics()
            elif command[0] == 'P':
                self.do_provision(command)
            elif command[0] == 'R':
                self.do_retrieve(command)
            elif command[0] == '#':
                self.do_rng()
            else:
                self.do_error(command)
        return len(data)

    def read(self,sz):
        if len(self.outbuf) >= sz:
            rv, self.outbuf = self.outbuf[:sz], self.outbuf[sz:]
            return rv
        if self.timeout > 0:
            time.sleep(self.timeout)
        rv, self.outbuf = self.outbuf, ''
        return rv


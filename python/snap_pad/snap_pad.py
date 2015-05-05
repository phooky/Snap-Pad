#!/usr/bin/python

import serial
import serial.tools.list_ports
import logging
import re
import time
from base64 import b64encode,b64decode

# Vendor and product ID from http://pid.codes/1209/2400/
# Thanks to the people behind pid.codes!
vendor_id=0x1209
product_id=0x2400


#
# All commands are terminated by a newline character.
# Standard command summary:
#
# D                       - diagnostics
# #                       - produce 64 bytes of random data from the RNG
# Rblock,page,para,count  - retrieve (and zero) count paragraphs 
#                           starting at block,page,para.
#                           maximum count is 4. will wait for user button
#                           press before continuing.
# Pcount                  - provision (and zero) count paragraphs. snap-pad
#                           chooses next available paras. maximum count is 4.
#                           will wait for user button press before continuing.
#

# regexps for parsing preambles
preamble_re = re.compile('^---(BEGIN|USED) PARA ([0-9]+),([0-9]+),([0-9]+)---$')
end_re = re.compile('^---END PARA---$')


class Paragraph:
    def __init__(self,block,page,para):
        self.block = block
        self.page = page
        self.para = para
        self.used = False
        self.bits = ''

def open_serial(port_name):
    'Convenience method for opening a serial port and flushing any noise.'
    port = serial.Serial(port_name)
    # Clean up any trash left behind by modemmanager or other tools that
    # automatically attempt to communicate with serial devices.
    port.flushInput()
    port.write('\n')
    port.flush()
    time.sleep(0.1)
    port.timeout=1
    port.flushInput()
    return port

class SnapPad:
    def __init__(self,port,sn):
        if type(port) == str:
            self.sp = open_serial(port)
        else:
            self.sp = port
        self.sn = sn
        logging.debug("Found snap-pad with SN {0}".format(self.sn))
        # Read version
        self.__read_version()
        # Read diagnostics
        self.diagnostics = self.__read_diagnostics()


    def __read_version(self):
        'Read the version number from the pad and warn on variant builds.'
        self.sp.write('V\n')
        m = re.match('([0-9]+)\\.([0-9]+)([A-Z]?)$',self.sp.readline())
        if not m:
            raise Error("Could not retrieve version from Snap-Pad")
        self.major = int(m.group(1))
        self.minor = int(m.group(2))
        self.variant = m.group(3)
        if self.variant == 'F':
            raise Error('This Snap-Pad is loaded with the factory test firmware.')
        elif self.variant == 'D':
            logging.warning('This Snap-Pad is running the debug firmware; this is not safe!')
        elif self.variant == 'M':
            logging.error('This Snap-Pad is a test mockup and is not suitable for use for any purpose!')
        elif self.variant == '':
            pass # no variant, just fine
        else:
            raise Error('This Snap-Pad is running an unknown firmware variant.')

    def __read_diagnostics(self):
        'Explicitly read diagnostics from pad'
        self.sp.write("D\n")
        line = self.sp.readline().strip()
        assert line == '---BEGIN DIAGNOSTICS---'
        d = {}
        while True:
            line = self.sp.readline().strip()
            if line == '---END DIAGNOSTICS---':
                break;
            try:
                [k,v] = line.split(":",1)
                if k == 'ERROR':
                    logging.error('Diagnostic error: {0}'.format(v))
                else:
                    d[k] = v
            except ValueError:
                logging.error('Unrecognized diagnostic: {0}'.format(line))
        return d

    def __is_single(self):
        "Return true if the snap-pad is disconnected from its twin"
        return self.diagnostics['Mode'] == 'Single board'

    def __read_para(self):
        'Helper for reading a paragraph from the device.'
        preamble = self.sp.readline()
        prematch = preamble_re.match(preamble)
        assert prematch
        para_type = prematch.group(1)
        assert para_type == 'USED' or para_type == 'BEGIN'
        block = int(prematch.group(2))
        assert block > 0 and block < 2048
        page = int(prematch.group(3))
        assert page >= 0 and page < 64
        para = int(prematch.group(4))
        assert para >= 0 and para < 4
        p = Paragraph(block,page,para)
        if para_type == 'USED':
            p.used = True
        elif para_type == 'BEGIN':
            data = ''
            while True:
                l=self.sp.readline()
                if end_re.match(l):
                    break
                data = data + l.strip()
            p.bits = b64decode(data)
        return p
        
    def retrieve_paragraphs(self,paragraphs):
        "Retrieve and zero a specified set of paragraphs"
        assert len(paragraphs) > 0 and len(paragraphs) <= 4
        for (block,page,para) in paragraphs:
            assert block > 0 and block < 2048
            assert page >= 0 and page < 64
            assert para >= 0 and para < 4
        specifiers = ["{0},{1},{2}".format(bl,pg,pr) for (bl,pg,pr) in paragraphs]
        command = "R"+",".join(specifiers)+"\n"
        self.sp.write(command)
        # Paragraphs begin with "---BEGIN PARA B,P,P---" and end with "---END PARA---"
        # If the block is already consumed, it emits "---USED PARA B,P,P---" instead of
        # either message
        paras = [self.__read_para() for _ in range(len(specifiers))]
        return paras

    def provision_paragraphs(self,count):
        "Provision the given count of paragraphs"
        assert count > 0 and count <= 4
        command = "P{0}\n".format(count)
        self.sp.write(command)
        paras = [self.__read_para() for _ in range(count)]
        return paras        

    def hwrng(self):
        'Return 64 bytes of random data from the hardware RNG'
        self.sp.write('#\n')
        return self.sp.read(64)

def add_pad_arguments(parser):
    'Add parser arguments for finding a snap-pad'
    parser.add_argument("-s", "--serial", type=str,
                        help="indicate serial number of snap-pad")

def list_snap_pads():
    return [(x['port'],x['iSerial']) for x in serial.tools.list_ports.list_ports_by_vid_pid(vendor_id,product_id)]

def find_our_pad(args):
    'Find the snap-pad specified by the user on the command line'
    pads = list_snap_pads()
    if args.serial:
        for (port,serial) in pads:
            if serial == args.serial:
                return SnapPad(port)
        logging.error('No Snap-Pad matching serial number {0} found.'.format(args.serial))
        return None
    else:
        if len(pads) == 0:
            logging.error('No Snap-Pads detected; check that your Snap-Pad is plugged in.')
        elif len(pads) > 1:
            logging.error('Multiple Snap-Pads detected; use the --serial option to choose one.')
        else:
            return pads[0]
        return None


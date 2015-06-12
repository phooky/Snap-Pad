#!/usr/bin/python

import serial
import serial.tools.list_ports
import logging
import re
import time
from base64 import b64encode,b64decode
from snap_pad_vid_pid import vendor_id, product_id
from serial_util import find_snap_pads

#
# All commands are terminated by a newline character.
# Standard command summary:
#
# D                       - diagnostics
# #                       - produce 64 bytes of random data from the RNG
# Rpage[,page,page,page]  - retrieve (and zero) specified pages up to a
#                           maximum of 4 pages. will wait for user button
#                           press before continuing.
# Pcount                  - provision (and zero) count pages. snap-pad
#                           chooses next available pages. maximum count is 4.
#                           will wait for user button press before continuing.
#

# regexps for parsing preambles
preamble_re = re.compile('^---(BEGIN|USED) PAGE ([0-9]+)---$')
end_re = re.compile('^---END PAGE---$')


class Page:
    def __init__(self,page):
        self.page = page
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

    def __read_page(self):
        'Helper for reading a page from the device.'
        preamble = self.sp.readline()
        prematch = preamble_re.match(preamble)
        assert prematch
        page_type = prematch.group(1)
        assert page_type == 'USED' or page_type == 'BEGIN'
        page = int(prematch.group(2))
        assert page > 0 and page < (2048*64)
        p = Page(page)
        if page_type == 'USED':
            p.used = True
        elif page_type == 'BEGIN':
            data = ''
            while True:
                l=self.sp.readline()
                if end_re.match(l):
                    break
                data = data + l.strip()
            p.bits = b64decode(data)
            assert len(p.bits) == 2048
        return p
        
    def retrieve_pages(self,pages):
        "Retrieve and zero a specified set of pages"
        assert len(pages) > 0 and len(pages) <= 4
        for page in pages:
            assert page > 0 and page < (2048*64)
        command = "R"+",".join(map(str,pages))+"\n"
        self.sp.write(command)
        return [self.__read_page() for _ in range(len(pages))]

    def provision_pages(self,count):
        "Provision the given count of pages"
        assert count > 0 and count <= 4
        command = "P{0}\n".format(count)
        self.sp.write(command)
        pages = [self.__read_page() for _ in range(count)]
        return pages        

    def hwrng(self):
        'Return 64 bytes of random data from the hardware RNG'
        self.sp.write('#\n')
        return self.sp.read(64)

def add_pad_arguments(parser):
    'Add parser arguments for finding a snap-pad'
    parser.add_argument('--sn', type=str,
            help='use the Snap-Pad with the specified serial number')
    parser.add_argument('--port', type=str,
            help='use the Snap-Pad attached to the specified port')

def list_snap_pads():
    return find_snap_pads()
    
def find_our_pad(args):
    'Find the snap-pad specified by the user on the command line'
    pads = list_snap_pads()
    if args.sn:
        for (port,sn) in pads:
            if sn == args.sn:
                return SnapPad(port)
        logging.error('No Snap-Pad matching serial number {0} found.'.format(args.serial))
        return None
    elif args.port:
        return SnapPad(args.port)
    else:
        if len(pads) == 0:
            logging.error('No Snap-Pads detected; check that your Snap-Pad is plugged in.')
        elif len(pads) > 1:
            logging.error('Multiple Snap-Pads detected; use the --sn or --port option to choose one.')
        else:
            return pads[0]
        return None


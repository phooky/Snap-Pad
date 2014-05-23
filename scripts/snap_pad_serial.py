#!/usr/bin/python

import serial
import serial.tools.list_ports
import logging
import re
from base64 import b64encode,b64decode

vendor_id=0x2047
product_id=0x03ee


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
preamble_re = re.compile("^---(BEGIN|USED) PARA ([0-9]+),([0-9]+),([0-9]+)---$")
end_re = re.compile("^---END PARA---$")

def find_snap_pads():
    return [SnapPad(x['port'],x['iSerial']) for x in serial.tools.list_ports.list_ports_by_vid_pid(vendor_id,product_id)]

class Paragraph:
    def __init__(self,block,page,para):
        self.block = block
        self.page = page
        self.para = para
        self.used = False
        self.bits = ''

class SnapPad:
    def __init__(self,port,sn):
        self.p = serial.Serial(port)
        self.sn = sn
        logging.debug("Found snap-pad with SN {0}".format(self.sn))
        # Read diagnostics
        self.diagnostics = self.read_diagnostics()

    def read_diagnostics(self):
        "Explicitly read diagnostics from pad"
        self.p.flushInput()
        self.p.write("D\n")
        line = self.p.readline().strip()
        assert line == '---BEGIN DIAGNOSTICS---'
        d = {}
        while True:
            line = self.p.readline().strip()
            if line == '---END DIAGNOSTICS---':
                break;
            [k,v] = line.split(":",1)
            d[k] = v
        self.insecure = d.has_key("Debug")
        if self.insecure:
            logging.warning("Debugging build; pad {0} is not secure!!!".format(self.sn))
        return d

    def is_single(self):
        "Return true if the snap-pad is disconnected from its twin"
        return self.diagnostics['Mode'] == 'Single board'

    def read_para(self):
        preamble = self.p.readline()
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
                l=self.p.readline()
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
        self.p.flushInput()
        self.p.write(command)
        # Paragraphs begin with "---BEGIN PARA B,P,P---" and end with "---END PARA---"
        # If the block is already consumed, it emits "---USED PARA B,P,P---" instead of
        # either message
        paras = [self.read_para() for _ in range(len(specifiers))]
        return paras

    def provision_paragraphs(self,count):
        "Provision the given count of paragraphs"
        assert count > 0 and count <= 4
        command = "P{0}\n".format(count)
        self.p.flushInput()
        self.p.write(command)
        paras = [self.read_para() for _ in range(count)]
        return paras        

def add_pad_arguments(parser):
    "Add parser arguments for finding a snap-pad"
    parser.add_argument("-s", "--serial", type=str,
                        help="indicate serial number of snap-pad")

def find_our_pad(args):
    "Find the snap-pad specified by the user on the command line"
    pads = find_snap_pads()
    if args.serial:
        for pad in pads:
            if pad.sn == args.serial:
                return pad
        logging.error("No Snap-Pad matching serial number {0} found!!!",args.serial)
        return None
    else:
        if len(pads) == 0:
            logging.error("No Snap-Pads detected")
        elif len(pads) > 1:
            logging.error("Multiple Snap-Pads detected; use the --serial option to choose one")
        else:
            return pads[0]
        return None

        
if __name__ == '__main__':
    # enumerate pads
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbosity", action="count",
                        default=0,
                        help="increase output verbosity")
    parser.add_argument("-r", "--retrieve", action="append",
                        help="retrieve a 'block,page,para' triplet")
    parser.add_argument("-p", "--provision", type=int,
                        help="retrieve a 'block,page,para' triplet")
    args = parser.parse_args()
    logging.getLogger().setLevel(logging.INFO - 10*args.verbosity)
    pads = find_snap_pads()
    for pad in pads:
        d = pad.diagnostics
        if not pad.is_single():
            logging.warning("This pad is not snapped!")
        if args.retrieve > 0:
            paras = pad.retrieve_paragraphs([tuple(map(int,r.split(","))) for r in args.retrieve])
        if args.provision:
            paras = pad.provision_paragraphs(args.provision)
            for p in paras:
                print p.block,p.page,p.para,"used",p.used,"data size",len(p.bits)
                        

            
        
            
            

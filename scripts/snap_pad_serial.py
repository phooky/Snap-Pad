#!/usr/bin/python
import serial
import serial.tools.list_ports

import logging

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

def find_snap_pads():
    return [SnapPad(x['port'],x['iSerial']) for x in serial.tools.list_ports.list_ports_by_vid_pid(vendor_id,product_id)]
        
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
        self.p.write("D\n\r")
        line = self.p.readline().strip()
        assert line == '---BEGIN DIAGNOSTICS---'
        d = {}
        while True:
            line = self.p.readline().strip()
            if line == '---END DIAGNOSTICS---':
                break;
            [k,v] = line.split(":",1)
            d[k] = v
        if d.has_key("Debug"):
            logging.warning("Debugging build; pad {0} is not secure".format(self.sn))
        return d

    def is_single(self):
        "Return true if the snap-pad is disconnected from its twin"
        return self.diagnostics['Mode'] == 'Single board'

if __name__ == '__main__':
    # enumerate pads
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbosity", action="count",
                        default=0,
                        help="increase output verbosity")
    args = parser.parse_args()
    logging.getLogger().setLevel(logging.INFO - 10*args.verbosity)
    pads = find_snap_pads()
    for pad in pads:
        d = pad.diagnostics
        print pad.is_single()

            
        
            
            

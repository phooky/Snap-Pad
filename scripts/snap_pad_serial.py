#!/usr/bin/python
import serial
import serial.tools.list_ports

vendor_id=0x2047
product_id=0x03ee

def find_snap_pads():
    return [SnapPad(x['port'],x['iSerial']) for x in serial.tools.list_ports.list_ports_by_vid_pid(vendor_id,product_id)]
        
class SnapPad:
    def __init__(self,port,sn):
        self.p = serial.Serial(port)
        self.sn = sn
        print "Found pad with SN {0}".format(self.sn)

print find_snap_pads()

            
            
        
            
            

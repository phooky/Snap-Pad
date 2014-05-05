#!/usr/bin/python
import usb.core

vendor_id=0x2047
product_id=0x03ee

def find_pad(sn=None):
    """
    Return a usb device representing a Snap-Pad with the given serial 
    number. If no serial number is provided and only one Snap-Pad is
    present, use that Snap-Pad.
    """
    pads = usb.core.find(idVendor=vendor_id,idProduct=product_id,find_all=True)
    if not pads:
        raise RuntimeError("No Snap-Pads found")
    if sn:
        for p in pads:
            if p.serial_number() == sn:
                return p
        raise RuntimeError("No Snap-Pad with serial # {0} found".format(sn))
    else:
        if len(pads) > 1:
            raise RuntimeError("Multiple Snap-Pads found; specify serial #")
        return pads[0]

class SnapPad:
    def __init__(self,sn=None):
        self.d = find_pad(sn)
        print "Found pad with SN {0}".format(self.d.serial_number())

            
            
        
            
            

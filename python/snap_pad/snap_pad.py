#!/usr/bin/python

import serial
import serial.tools.list_ports
import logging
import re
import time
from containers import EncryptedMessage
from base64 import b64decode
from snap_pad_config import vendor_id, product_id, PAGESIZE, VERSION
from serial_util import find_snap_pads
import array
import hmac
import math
from struct import pack, unpack
from hashlib import sha256

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

# Exceptions
class SnapPadTimeoutException(Exception):
    'Indicates a pad timeout, usually because the user has not pressed the button'
    pass

class BadSignatureException(Exception):
    'Indicates that at least one part of the signature could not be verified'
    def __init__(self,bad_data,page_oks):
        self.bad_count = len(page_oks) - sum(page_oks)
        Exception.__init__(self,'Bad signatures found on {0} of {1} pages'.format(self.bad_count,len(page_oks)))
        self.bad_data = bad_data
        self.page_oks = page_oks

def pages_needed_for(data):
    return int(math.ceil(len(data)/float(PAGESIZE)))



class Page:
    "One 2048-byte page of random data from a pad"
    def __init__(self,page_idx):
        self.page_idx = page_idx
        self.used = True
        self.bits = None

    def set_bits(self, bits_as_str):
        self.used = False
        self.bits = array.array('B',bits_as_str)

    def crypt(self,data):
        'en/decrypt data'
        assert len(data) <= PAGESIZE
        assert self.used == False
        assert len(self.bits) == 2048
        inb = array.array('B',data)
        outb = array.array('B')
        for i in range(len(inb)):
            outb.append( inb[i] ^ self.bits[i] )
        return outb.tostring()

    def sign(self,data):
        'compute signature of data'
        salt_bytes = self.bits[PAGESIZE:PAGESIZE+16]
        mac_crypt_bytes = self.bits[PAGESIZE+16:]
        assert len(salt_bytes) == 16
        assert len(mac_crypt_bytes) == 32
        mac = array.array('B',hmac.new(salt_bytes.tostring(),data,sha256).digest())
        assert len(mac) <= len(mac_crypt_bytes)
        outmac = array.array('B')
        for i in range(len(mac)):
            outmac.append(mac_crypt_bytes[i] ^ mac[i])
        return outmac.tostring()

    def verify(self,data,sig):
        computed = self.sign(data)
        return computed == sig

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
        # Timeout for this read is longer
        old_timeout, self.sp.timeout = self.sp.timeout, 11
        preamble = self.sp.readline()
        self.sp.timeout = old_timeout
        prematch = preamble_re.match(preamble)
        if not prematch:
            if preamble.strip() == '---TIMEOUT---':
                raise SnapPadTimeoutException
            else:
                print("WHAT? {0}".format(preamble))
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
            p.set_bits(b64decode(data))
            assert len(p.bits) == 2048
        return p
        
    def retrieve_pages(self,page_idxs):
        "Retrieve and zero a specified set of pages"
        assert len(page_idxs) > 0 and len(page_idxs) <= 4
        for page_idx in page_idxs:
            assert page_idx > 0 and page_idx < (2048*64)
        command = "R"+",".join(map(str,page_idxs))+"\n"
        self.sp.write(command)
        return [self.__read_page() for _ in range(len(page_idxs))]

    def provision_pages(self,count):
        "Provision the given count of pages"
        assert count > 0 and count <= 4
        command = "P{0}\n".format(count)
        self.sp.write(command)
        pages = [self.__read_page() for _ in range(count)]
        # TODO: Handle timeouts, other failures
        return pages 


    def encrypt_and_sign(self,raw_message):
        "Encrypt and sign plaintext and return an encrypted message"
        data = self.marshall(raw_message)
        assert len(data) <= PAGESIZE*4
        page_count = pages_needed_for(data)
        pages = self.provision_pages(page_count)
        pages.reverse()
        msg = EncryptedMessage()
        while len(data) > 0:
            chunk,data = data[:PAGESIZE],data[PAGESIZE:]
            page = pages.pop()
            enc_data = page.crypt(chunk)
            enc_sig = page.sign(enc_data)
            msg.add_block(page.page_idx,enc_data,enc_sig)
        return msg

    def decrypt_and_verify(self,msg):
        "Decrypt and verify an encrypted message, and return a decrypted message object"
        assert len(msg.blocks) <= 4
        page_numbers = [x[0] for x in msg.blocks]
        pages = { p.page_idx:p for p in self.retrieve_pages(page_numbers) }
        decrypted = b''
        sig_verified = []
        for (page_idx, data, sig) in msg.blocks:
            page = pages[page_idx]
            decrypted = decrypted + page.crypt(data)
            sig_verified.append( page.verify(data, sig) )
        all_ok = reduce(lambda x,y:x and y, sig_verified)
        if all_ok:
            # unmarshall
            decrypted = self.unmarshall(decrypted)
            return decrypted
        else:
            raise BadSignatureException(decrypted,sig_verified)


    def hwrng(self):
        'Return 64 bytes of random data from the hardware RNG'
        self.sp.write('#\n')
        return self.sp.read(64)

    def marshall(self,raw_message):
        '''Marshalls a message prior to encryption. This consists of prepending a 16-bit
        message size and then padding the message to the next block boundry.'''
        data = pack('>H',len(raw_message)) + raw_message
        pad_to = pages_needed_for(data) * PAGESIZE
        while pad_to > len(data):
            sz = pad_to - len(data)
            pad_bytes = self.hwrng()
            data += pad_bytes[:sz]
        return data

    def unmarshall(self,marshalled_message):
        '''Strip the padding and message size from a marshalled message and return the raw
        message.'''
        [msg_sz] = unpack('>H',marshalled_message[:2])
        message = marshalled_message[2:2+msg_sz]
        return message


def list_snap_pads(args):
    if args.mock_pad:
        return [('MOCK','MOCK')]
    return find_snap_pads()

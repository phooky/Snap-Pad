#!/usr/bin/python
from base64 import b64encode,b64decode
from textwrap import fill
from struct import pack, unpack, calcsize
from cStringIO import StringIO
import json



BIN_HEADER_MAGIC='SP-BLOCK'
BIN_HEADER_FMT = '<8sHH32s'
BIN_HEADER_SZ = calcsize(BIN_HEADER_FMT)

JSON_MAGIC='Snap-Pad OTP Message'
from snap_pad_config import VERSION, PAGESIZE

class AsciiParseError:
    pass

class VersionError:
    pass

class EncryptedMessage:
    'An encrypted message.'
    def __init__(self):
        self.blocks = []

    def add_block(self, page_idx, data, sig):
        self.blocks.append( (page_idx, data, sig))

    def write_ascii(self, f):
        msg_obj = { 'Version':VERSION, 'Magic':JSON_MAGIC }
        msg_obj['Blocks'] = [{'Page':page_idx, 'Signature':b64encode(sig), 'Data':b64encode(data)} for 
            (page_idx,data,sig) in self.blocks]
        return json.dump(msg_obj,f)

    def read_ascii(self,f):
        msg_obj = json.read(f)
        try:
            assert msg_obj['Magic'] == JSON_MAGIC
        except:
            raise AsciiParseError('Missing or incorrect magic field')
        try:
            assert msg_obj['Version'] == VERSION
        except:
            raise VersionError('Unrecognized or missing version')
        for b in msg_obj['Blocks']:
            page_idx = b['Page']
            data = b64decode(b['Data'])
            sig = b64decode(b['Signature'])
            self.add_block(page_idx,data,sig)

    def write_binary(self,f):
        for (page_idx, data, sig) in self.blocks:
            f.write(pack(BIN_HEADER_FMT, BIN_HEADER_MAGIC, page_idx, len(data), sig))
            f.write(data)

    def read_binary(self,f):
        while True:
            header = f.read(BIN_HEADER_SZ)
            if len(header) == 0:
                break
            assert len(header) == BIN_HEADER_SZ
            (magic, page_idx, sz, sig) = unpack(BIN_HEADER_FMT,header)
            assert magic == BIN_HEADER_MAGIC
            assert sz <= PAGESIZE
            data = f.read(sz)
            self.add_block( page_idx, data, sig )

    def to_ascii(self):
        s = StringIO()
        self.write_ascii(s)
        rv = s.getvalue()
        s.close()
        return rv

    def to_binary(self):
        s = StringIO()
        self.write_binary(s)
        rv = s.getvalue()
        s.close()
        return rv

    def from_ascii(f):
        e = EncryptedMessage()
        e.read_ascii(f)
        return e

    def from_binary(f):
        e = EncryptedMessage()
        e.read_binary(f)
        return e

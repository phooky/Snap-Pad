#!/usr/bin/python
from base64 import b64encode,b64decode
from textwrap import fill
from struct import pack, unpack, calcsize
from cStringIO import StringIO
import json
import re

BIN_HEADER_MAGIC='SP-BLOCK'
BIN_HEADER_FMT = '<8sHH32s'
BIN_HEADER_SZ = calcsize(BIN_HEADER_FMT)

JSON_MAGIC='Snap-Pad OTP Message'
from snap_pad_config import VERSION, PAGESIZE

ascii_begin_str = '-----BEGIN SNAP-PAD MESSAGE BLOCK-----'
ascii_end_str = '-----END SNAP-PAD MESSAGE BLOCK------'

class AsciiParseError:
    pass

class VersionError:
    pass

class EncryptedMessage:
    'An encrypted message.'
    def __init__(self):
        self.blocks = []

    def __eq__(self,other):
        if not len(self.blocks) == len(other.blocks): return False
        for i in range(len(self.blocks)):
            if not self.blocks[i] == other.blocks[i]: return False
        return True

    def add_block(self, page_idx, data, sig):
        self.blocks.append( (page_idx, data, sig))

    def write_json(self, f):
        msg_obj = { 'Version':VERSION, 'Magic':JSON_MAGIC }
        msg_obj['Blocks'] = [{'Page':page_idx, 'Signature':b64encode(sig), 'Data':b64encode(data)} for 
            (page_idx,data,sig) in self.blocks]
        return json.dump(msg_obj,f)

    def read_json(self,f):
        msg_obj = json.load(f)
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

    def write_ascii(self,f):
        bin_data = self.to_binary()
        f.write(ascii_begin_str+'\n')
        f.write('Version: {0}\n'.format(VERSION))
        f.write(fill(b64encode(bin_data)))
        f.write(ascii_end_str+'\n')

    def read_ascii(self,f):
        header = f.readline().strip()
        if header != ascii_begin_str:
            raise AsciiParseError('Expected header')
        line = f.readline().strip()
        ver = re.match('Version: (\w+)',line)
        if ver:
            if ver.group(1) != VERSION:
                raise AsciiParseError('Unknown version')
            line = f.readline().strip()
        else:
            # No version information; accept
            pass
        b64data = ''
        while line != ascii_end_str:
            if line == '':
                raise AsciiParseError('Unexpected end of data')
            else:
                b64data += line
        f = StringIO(b64decode(b64data))
        self.read_binary(f)

    def _to_helper(self,method):
        s = StringIO()
        method(s)
        rv = s.getvalue()
        s.close()
        return rv


    def to_json(self):
        return self._to_helper(self.write_json)

    def to_binary(self):
        return self._to_helper(self.write_binary)

    def to_ascii(self):
        return self._to_helper(self.write_ascii)

    @staticmethod
    def _from_helper(method,f):
        if type(f) == str:
            f = StringIO(f)
        e = EncryptedMessage()
        method(e,f)
        return e

    @staticmethod
    def from_json(f):
        return EncryptedMessage._from_helper(EncryptedMessage.read_json,f)

    @staticmethod
    def from_binary(f):
        return EncryptedMessage._from_helper(EncryptedMessage.read_binary,f)

    @staticmethod
    def from_ascii(f):
        return EncryptedMessage._from_helper(EncryptedMessage.read_ascii,f)

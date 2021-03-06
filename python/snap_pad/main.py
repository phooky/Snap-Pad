#!/usr/bin/python
from snap_pad import SnapPad, list_snap_pads, PAGESIZE, EncryptedMessage

from cStringIO import StringIO
import snap_pad
import logging
import argparse
import sys
import os
import hashlib
import base64
import os.path

class FileExistsError(Exception):
    pass

class MessageTooLargeError(Exception):
    pass

class NoDataError(Exception):
    pass

class NoPadError(Exception):
    pass

def find_our_pad(args):
    'Find the snap-pad specified by the user on the command line'
    if args.mock_pad:
        from test.test_snap_pad_mock import SnapPadHWMock
        return SnapPad(SnapPadHWMock(),'MOCK')
    pads = list_snap_pads(args)
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
            raise NoPadError()
        elif len(pads) > 1:
            logging.error('Multiple Snap-Pads detected; use the --sn or --port option to choose one.')
            raise NoPadError()
        else:
            return SnapPad(pads[0][0],pads[0][1])
        return None

def get_output(args):
    if args.output == '-':
        return sys.stdout
    else:
        if os.path.exists(args.output):
            raise FileExistsError(args.output)
        if args.encoding == 'bin':
            mode = 'wb'
        else:
            mode = 'w'
        return open(args.output, mode)

def get_input(args):
    inf = sys.stdin
    indata = inf.read((PAGESIZE*4)+1)
    if len(indata) > (PAGESIZE*4 - 2):
        raise MessageTooLargeError()
    if len(indata) == 0:
        raise NoDataError()
    return indata

def parse_input(args,data):
    out = StringIO(data)
    if args.encoding == 'bin':
        return EncryptedMessage.from_binary(out)
    elif args.encoding == 'json':
        return EncryptedMessage.from_json(out)
    elif args.encoding == 'ascii':
        return EncryptedMessage.from_ascii(out)
    elif args.encoding == 'automatic':
        try:
            return EncryptedMessage.from_ascii(out)
        except:
            pass
        out = StringIO(data)
        try:
            return EncryptedMessage.from_json(out)
        except:
            pass
        out = StringIO(data)            
        return EncryptedMessage.from_binary(out)
    else:
        raise Exception('Bad encoding {0}'.format(args.encoding))

# subcommands:
# - list
# - diagnostics
# - random
# * encode
# * decode
# * help
def list_handler(args):
    pads = list_snap_pads(args)
    sys.stdout.write('{0} Snap-Pads detected.\n'.format(len(pads)))
    for (port,sn) in pads:
        sys.stdout.write('Port: {0} SN: {1}\n'.format(port,sn))

def random_handler(args):
    sp = find_our_pad(args)
    if not sp:
        return
    data = sp.hwrng()
    if args.bin:
        sys.stdout.write(data)
    else:
        from base64 import b64encode
        sys.stdout.write(b64encode(data))
        sys.stdout.write('\n')

def diagnostics_handler(args):
    sp = find_our_pad(args)
    if not sp:
        return
    diag = sp.diagnostics
    keys = diag.keys()
    keys.sort()
    for key in keys:
        sys.stdout.write('{0}: {1}\n'.format(key,diag[key]))

def encode_handler(args):
    sp = find_our_pad(args)
    out = get_output(args)
    raw_input = get_input(args)
    msg = sp.encrypt_and_sign(raw_input)
    if args.encoding == 'bin':
        msg.write_binary(out)
    elif args.encoding == 'json':
        msg.write_json(out)
    elif args.encoding == 'ascii' or args.encoding == 'automatic':
        msg.write_ascii(out)
    else:
        raise Exception('Bad encoding')

def decode_handler(args):
    sp = find_our_pad(args)
    out = get_output(args)
    raw_input = get_input(args)
    msg = parse_input(args,raw_input)
    data = sp.decrypt_and_verify(msg)
    out.write(data)

def make_parser():
    p = argparse.ArgumentParser()
    p.add_argument('--sn', type=str,
            help='use the Snap-Pad with the specified serial number')
    p.add_argument('--port', type=str,
            help='use the Snap-Pad attached to the specified port')
    p.add_argument('--mock_pad', action='store_true',
            help='use a mockup Snap-Pad [FOR TESTING ONLY]')
    sub = p.add_subparsers()
    p_list = sub.add_parser('list') #, aliases=['l'])
    p_list.set_defaults(handler=list_handler)
    p_rand = sub.add_parser('random') #, aliases=['rand','r'])
    p_rand.add_argument('--bin', action='store_true')
    p_rand.set_defaults(handler=random_handler)
    p_diag = sub.add_parser('diagnostics') #', aliases=['diag'])
    p_diag.set_defaults(handler=diagnostics_handler)
    p_enc = sub.add_parser('encode') #, aliases=['enc','e'])
    p_enc.set_defaults(handler=encode_handler)
    p_dec = sub.add_parser('decode') #, aliases=['dec','d'])
    p_dec.set_defaults(handler=decode_handler)
    for p_sub in [p_enc,p_dec]:
        p_sub.add_argument('--bin',action='store_const',dest='encoding',const='bin')
        p_sub.add_argument('--ascii',action='store_const',dest='encoding',const='ascii')
        p_sub.add_argument('--json',action='store_const',dest='encoding',const='json')
        p_sub.set_defaults(encoding='automatic')
        # output redirect
        p_sub.add_argument('-o',dest='output',default='-')
    return p

def main():
    parser = make_parser()
    args = parser.parse_args()
    args.handler(args)

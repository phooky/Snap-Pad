#!/usr/bin/python
from snap_pad import SnapPad, list_snap_pads, PAGESIZE

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
    elif args.encoding == 'ascii':
        msg.write_ascii(out)
    else:
        raise Exception('Bad encoding')


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
    for p_sub in [p_enc,p_dec]:
        p_sub.add_argument('--bin',action='store_const',dest='encoding',const='bin')
        p_sub.add_argument('--ascii',action='store_const',dest='encoding',const='ascii')
        p_sub.add_argument('--json',action='store_const',dest='encoding',const='json')
        p_sub.set_defaults(encoding='ascii')
        # output redirect
        p_sub.add_argument('-o',dest='output',default='-')
    return p

def main():
    parser = make_parser()
    args = parser.parse_args()
    args.handler(args)

def main_old():
    # enumerate pads
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbosity", action="count",
                        default=0,
                        help="increase output verbosity")
    parser.add_argument("-o", "--output", type=str,
                        default=None,
                        help="write output to specified file")
    parser.add_argument("-u", "--unsigned", action="store_true",
                        default=False,
                        help="do not sign message with MAC")
    parser.add_argument("-c", "--count", type=int,
                        default=0,
                        help="pad input to the given number of 2000B pages")
    parser.add_argument("--hash", type=str, default="SHA-256",
                        help="hash algorithm for MAC")
    parser.add_argument('FILE',nargs='*',
                        default=None,
                        help="files to encrypt (encrypts stdin if none given)")
    add_pad_arguments(parser)
    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)s: %(message)s')
    logging.getLogger().setLevel(logging.INFO - 10*args.verbosity)
    pad = find_our_pad(args)
    if not pad:
        sys.exit(1)
    output = sys.stdout
    if args.output:
        output = open(args.output,'w')
    if args.FILE:
        for path in args.FILE:
            fsize = os.stat(path).st_size
            if fsize < PAGESIZE*4:
                f = open(path,'r')
                encrypt(pad,f,output)
    else:
        encrypt(pad,sys.stdin,output)

    

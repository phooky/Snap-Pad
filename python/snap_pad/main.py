#!/usr/bin/python
from snap_pad import add_pad_arguments, list_snap_pads, find_our_pad, PAGESIZE

import snap_pad
import logging
import argparse
import sys
import os
import hashlib
import base64

# sn-enc.py encrypts a message with a snap-pad.
def create_mac():
    pass

def encrypt(pad,inf,outf):
    indata = inf.read((PAGESIZE*4)+1)
    if len(indata) > PAGESIZE*4:
        # TODO: too big!
        pass
    blocks = pad.encrypt_and_sign(indata)
    for block in blocks:
        print block.ascii_armor()


# subcommands:
# * list
# * diagnostics
# * random
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

def make_parser():
    p = argparse.ArgumentParser()
    add_pad_arguments(p)
    sub = p.add_subparsers()
    p_list = sub.add_parser('list') #, aliases=['l'])
    p_list.set_defaults(handler=list_handler)
    p_rand = sub.add_parser('random') #, aliases=['rand','r'])
    p_rand.add_argument('--bin', action='store_true')
    p_rand.set_defaults(handler=random_handler)
    p_diag = sub.add_parser('diagnostics') #', aliases=['diag'])
    p_enc = sub.add_parser('encode') #, aliases=['enc','e'])
    # output redirect
    p_dec = sub.add_parser('decode') #, aliases=['dec','d'])
    # output redirect
    for p_sub in [p_enc,p_dec]:
        p_sub.add_argument('--bin',action='store_const',dest='encoding',const='bin')
        p_sub.add_argument('--ascii',action='store_const',dest='encoding',const='ascii')
        p_sub.add_argument('--json',action='store_const',dest='encoding',const='json')
        p_sub.set_defaults(encoding='ascii')
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

    

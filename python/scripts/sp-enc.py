#!/usr/bin/python
from snap_pad_serial import add_pad_arguments, find_our_pad
import logging
import argparse
import sys
import os
import hashlib

# sn-enc.py encrypts a message with a snap-pad.

def create_mac():
def encrypt(pad,inf,outf):
    pass

if __name__ == '__main__':
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
                        help="pad input to the given number of 512B blocks")
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
            if fsize < 512*4:
                f = open(path,'r')
                encrypt(pad,f,output)
    else:
        encrypt(pad,sys.stdin,output)

    
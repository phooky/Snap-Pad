#!/usr/bin/python
from snap_pad_serial import add_pad_arguments, find_our_pad
import logging
import argparse
import sys

# sn-enc.py encrypts a message with a snap-pad.


if __name__ == '__main__':
    # enumerate pads
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbosity", action="count",
                        default=0,
                        help="increase output verbosity")
    add_pad_arguments(parser)
    args = parser.parse_args()
    logging.basicConfig(format='%(levelname)s: %(message)s')
    logging.getLogger().setLevel(logging.INFO - 10*args.verbosity)
    pad = find_our_pad(args)
    if not pad:
        sys.exit(1)
        
    print "Found pad",pad

Snap-Pad Software
=================

Uploading:
    sudo pip install python-msp430-tools
doesn't work; the pypi version hasn't been updated in a while.

Instead:
    bzr branch lp:python-msp430-tools
    cd python-msp430-tools
    sudo python setup.py install


CCSv5 install

Add MSP430 USB stuff:
    unzip the usb package somewhere (~/opt)
    Go to "resource explorer" in CCSv5
    add that folder to the resource explorer

Copy C3 example
Fix case problem in case hal_macros.h


GCC usage:
    msp430-gcc -mmcu=msp430f5508 MSP430F550x_P1_01.c 


Burn:
    sudo msp430-tool bsl5.hid -S --password=password.txt -i elf -v -P ~/Repos/Snap-Pad/code/SnapPadv0/Debug/SnapPadv0.out

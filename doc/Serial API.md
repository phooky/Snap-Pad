Snap-Pad USB Serial API
=======================

When a Snap-Pad is plugged into a USB port, it appears as a standard USB HID serial device.

[TODO: what the devices look like on Windows, OS X, Linux]

All commands sent to the Snap-Pad are terminated with a newline character ('\n').
Unrecognized or unparsable commands return a newline-terminated message starting with the string 'ERROR' and optionally containing additional information.

There are multiple versions of the Snap-Pad firmware available for production, debugging, and factory test use. All versions support the version ('V') command, which gives the version number and firmware variant.

Basic Snap-Pad Commands
-----------------------

* Version
  * Command: 'V'
  * Works on: all versions
  * Response: a newline-terminated string containing the version number of the firmware and an optional letter indicating the variant. For example, the string '1.1D' indicates a major version number of 1, a minor version of 1, and that the firmware in question is the debugging variant. The defined variants are:

[TODO: ADD VARIANT LETTERS, REFACTOR VERSION COMMAND OUT]

 Letter |   Variant
--------|-----------------------
 _None_ |  Production version
 D      |  Debug version; do not use in the wild!
 F      |  Factory test firmware


* Random bits
  * Command: '#'
  * Works on: all versions
  * Response: 16 bytes of random data directly from the hardware random number generator. Please note that this response is not encoded in any way; the bytes are written raw to the serial port.

* Get diagnostics
  * Command: 'D'
  * Works on: debug and production
  * Response: A brief diagnostic message of the form
  [TODO: REFACTOR DIAGNOSTICS] 
            ---BEGIN DIAGNOSTICS---
            Debug: true
            Mode: Single board
            Random:Done
            Blocks:2047
            ---END DIAGNOSTICS---
            
* Retrieve paragraphs
  * Command: 'R(block#),(page#),(paragraph#)[,(block#),(page#),(paragraph#)]'
  * Works on: debug and production
  * The retrieve command is used to read and zero specific paragraphs from the Snap-Pad, ordinarily for decrypting a message. Up to four paragraphs may be requested in a single command. When the command is issued, 1-4 LEDs will begin to blink on the Snap-Pad. The user can then push the button on the Snap-Pad to retrieve the paragraphs. They will be returned as base64 encoded data. If the user fails to push the button, a timeout message will be returned. After the data has been returned, the paragraph will be zeroed on the NAND flash.
  * Response: one base64 encoded block for each paragraph returned, fomatted as:
  
            ---BEGIN PARA (block#),(page#),(paragraph#)---
            [base64-encoded contents of paragraph]
            ---END PARA---

    or, if the requested block has already been used:

            ---USED PARA (block#),(page#),(paragraph#)---

    or, a timeout message (ATTN: NOT YET IMPLEMENTED):
    
            ---TIMEOUT---
            
* Provision paragraphs
  * Command: 'P(count#)'
  * Retrieve 1-4 paragraphs from the 'start' of the nand, normally to encrypt a new message. (Note that on one of the boards in a pair, the 'start' is actually the end of the nand!) When the command is issued, 1-4 LEDs will begin to blink on the Snap-Pad. The user can then push the button on the Snap-Pad to provision the paragraphs. They will be returned as base64 encoded data. If the user fails to push the button, a timeout message will be returned.
  * Response: one base64 encoded block for each paragraph returned, fomatted as:
  
            ---BEGIN PARA (block#),(page#),(paragraph#)---
            [base64-encoded contents of paragraph]
            ---END PARA---
    or, a timeout message (ATTN: NOT YET IMPLEMENTED):
    
            ---TIMEOUT---
  
Debug Commands
--------------

 *
 * Additional debug build commands:
 * C                        - print the bad block list
 * U                        - print the used block list
 * cblock                   - print the checksum of the indicated block
 * Mblock                   - mark the given block as used
 * F                        - find the address of the next block containing provisionable paras
 * rblock,page,para         - read the given block, page, and paragraph without erasing
 * Eblock                   - erase the indicated block (0xff everywhere)

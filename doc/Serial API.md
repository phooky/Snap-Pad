Snap-Pad USB Serial API
=======================

When a Snap-Pad is inserted into a USB port, it appears as a standard USB HID serial device. This document describes the interface for communicating with both unsnapped board pairs and snapped boards.

All commands sent to the Snap-Pad are terminated with a newline character ('\n').

Unrecognized commands presently yield the spectacularly unhelpful message "ERR\n".

Standard Commands
-----------------
 * Diagnostics
   * Command: 'D'
   * Response: A brief diagnostic message of the form
   
            ---BEGIN DIAGNOSTICS---
            Debug: true
            Mode: Single board
            Random:Done
            Blocks:2047
            ---END DIAGNOSTICS---
            
* Random bits
  * Command: '#'
  * Response: 64 bytes of random data directly from the RNG. Please note that this response is not encoded in any way; the bytes are written raw to the serial port.
  
* Retrieve paragraphs
  * Command: 'R(block#),(page#),(paragraph#)[,(block#),(page#),(paragraph#)]'
  * The retrieve command is used to read specific paragraphs from the Snap-Pad, ordinarily for decrypting a message. Up to four paragraphs may be requested in a single command. When the command is issued, 1-4 LEDs will begin to blink on the Snap-Pad. The user can then push the button on the Snap-Pad to retrieve the paragraphs. They will be returned as base64 encoded data. If the user fails to push the button, a timeout message will be returned.
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

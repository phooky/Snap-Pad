/* --COPYRIGHT--,BSD
 * Copyright (c) 2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*  
 * ======== main.c ========
 */
#include <string.h>

#include "driverlib.h"

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_Common/usb.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"

/*
 * NOTE: Modify hal.h to select a specific evaluation board and customize for
 * your own board.
 */
#include "hal.h"
#include "nand.h"
#include "hwrng.h"
#include "onetimepad.h"
#include "leds.h"
#include "uarts.h"
#include "config.h"
#include "print.h"
#include "timer.h"
#include "buffers.h"

// instantiate connection_state
uint8_t connection_state;

bool process_usb();

ConnectionState cs;
OTPConfig config;

void do_factory_reset_mode();
void do_twinned_master_mode();
void do_twinned_slave_mode();
void do_single_mode();

int main (void)
{
    // Set up clocks/IOs.  initPorts()/initClocks() will need to be customized
    // for your application, but MCLK should be between 4-25MHz.  Using the
    // DCO/FLL for MCLK is recommended, instead of the crystal.  For examples 
    // of these functions, see the complete USB code examples.  Also see the 
    // Programmer's Guide for discussion on clocks/power.
    WDT_A_hold(WDT_A_BASE); // Stop watchdog timer
    
    // Minimum Vcore setting required for the USB API is PMM_CORE_LEVEL_2 .
    PMM_setVCore(PMM_BASE, PMM_CORE_LEVEL_2);
    
    initPorts();             // Configure all GPIOs
    nand_init();             // Set up NAND pins
    leds_init();			 // Set up LED pins
    timer_init();			 // Set up msec timer
    hwrng_init();            // Initialize HW RNG
    initClocks(20000000);     // Configure clocks
    USB_setup(TRUE,TRUE);    // Init USB & events; if a host is present, connect
    uart_init();			 // Initialize uarts between hosts

    __enable_interrupt();    // Enable interrupts globally

    // If the button is pressed at startup time on a twinned board, note that fact: it means that we want to
    // start factory reset mode.
    bool button_pressed_on_startup = has_confirm();

    // Work out our configuration state. We can be:
    // Half pad and USB connected - SINGLE
    // Half pad and USB disconnected - SINGLE (don't care)
    // Twinned pad and USB connected - TWINNED_MASTER
    // Twinned pad and USB disconnected - TWINNED_SLAVE or TWINNED_MASTER (uart contention for role)

    // First, scan the USB connection for ~100ms to see if the device is being enumerated.
    bool usb_attached = false;
    timer_reset();
    while (timer_msec() < 600) {
    	uint8_t s = USB_connectionState();
        if (s == ST_ENUM_ACTIVE || s == ST_ENUM_SUSPENDED)  {
    		usb_attached = true;
    		break;
    	}
    }

    cs = uart_determine_state(usb_attached, button_pressed_on_startup);

	config = otp_read_header();

    if (cs == CS_TWINNED_MASTER) {
    	if (button_pressed_on_startup) {
    		do_factory_reset_mode();
    	} else {
    		do_twinned_master_mode();
    	}
    } else if (cs == CS_TWINNED_SLAVE) {
    	do_twinned_slave_mode();
    } else if (cs == CS_SINGLE) {
    	do_single_mode();
    }
}

void do_single_mode() {
	// Indicate ready
	// TODO: indicate exhausted
	if (config.randomization_finished) {
		leds_set_mode(LM_READY);
	} else {
		// error mode: partially programmed half!
		leds_set_mode(LM_DUAL_PARTIAL_PROG);
	}
    while (1)  // main loop
    {
        switch(USB_connectionState())
        {
            case ST_ENUM_ACTIVE:
            	process_usb();
            	break;
            case ST_USB_DISCONNECTED: // physically disconnected from the host
            case ST_ENUM_SUSPENDED:   // connecte d/enumerated, but suspended
            case ST_NOENUM_SUSPENDED: // connected, enum started, but the host is unresponsive
            case ST_ENUM_IN_PROGRESS:
            default:;
        }
    }
}

void do_twinned_master_mode() {
	// check otp header
	if (!config.has_header) {
		otp_initialize_header(true);
		config = otp_read_header();
	}
	// Go ahead to attract mode
	if (config.randomization_finished) {
		leds_set_mode(LM_DUAL_PROG_DONE);
	} else if (config.randomization_started) {
		leds_set_mode(LM_DUAL_PARTIAL_PROG);
	} else {
		leds_set_mode(LM_DUAL_NOT_PROG);
	}
	bool do_pings = true;
    while (!has_confirm() || !do_pings)  // waiting for button press, must replug if not
    {
    	int ucs = USB_connectionState();
    	if (ucs == ST_ENUM_ACTIVE) {
    		// quit pinging remote button once a USB command has been processed.
    		// (slow nand reads can cause very rare ping timeouts, and there's no need anyway.)
    		if (process_usb()) { do_pings = false; }
    	}
        // ping every 2ms if no usb commands have yet been received.
    	if (do_pings && timer_msec() >= 2) {
            if (uart_ping_button()) break;
        	timer_reset();
    	}
    }
    leds_set_mode(LM_DUAL_PROG_DONE);
    otp_randomize_boards();
}

void do_twinned_slave_mode() {
	// check otp header
	if (!config.has_header) {
		otp_initialize_header(false);
		config = otp_read_header();
	}
	// Go ahead to attract mode
	if (config.randomization_finished) {
		leds_set_mode(LM_DUAL_PROG_DONE);
	} else if (config.randomization_started) {
		leds_set_mode(LM_DUAL_PARTIAL_PROG);
	} else {
		leds_set_mode(LM_DUAL_NOT_PROG);
	}
    while (1)  // main loop
    {
    	uart_process(); // process uart commands
    }
}

void do_factory_reset_mode() {
	// Don't bother entering the main loop; go directly to reset confirmation mode
	uint8_t i;
	// Alternating side slow blink: confirm reset
	for (i = 0; i < LED_COUNT; i++) leds_set_led(i,LED_SLOW_0);
	uart_factory_reset_confirm();
	// Alternating leds on board: reset in process
	for (i = 0; i < LED_COUNT; i++) leds_set_led(i,(i%2 == 0)?LED_FAST_0:LED_FAST_1);
	otp_factory_reset();
	// Lights off: reset done
	for (i = 0; i < LED_COUNT; i++) leds_set_led(i,LED_OFF);
	while (1){} // Loop forever
}

// also used to de-decimal
uint8_t dehex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
	if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
	return 0;
}

void error(const char* msg) {
	print_usb_str("ERROR: ");
	if (msg) {
		print_usb_str(msg);
	}
	print_usb_str("\n");
}

void timeout() {
	print_usb_str("---TIMEOUT---\n");
}

void read_status() {
	char sbuf[8];
	uint8_t status = nand_read_status_reg();
	uint8_t i;
	for (i = 0; i < 8; i++) {
		sbuf[7-i] = ((status&0x01)==0)?'0':'1';
	}
	cdcSendDataWaitTilDone((BYTE*)sbuf, 8, CDC0_INTFNUM, 100);
	print_usb_str("\nOK\n");
}

void diagnostics() {
	// Display connection state
	print_usb_str("---BEGIN DIAGNOSTICS---\n");
#ifdef DEBUG
	print_usb_str("Debug: true\n");
#endif
	{
		print_usb_str("Mode: ");
		if (cs == CS_TWINNED_MASTER) { print_usb_str("Twinned master\n"); }
		else if (cs == CS_TWINNED_SLAVE) { print_usb_str("Twinned slave\n"); }
		else if (cs == CS_SINGLE) { print_usb_str("Single board\n"); }
		else { print_usb_str("CS_?\n"); }
	}
	// Check chip connection
	if (!nand_check_ONFI()) {
		error("ONFI failure");
		return;
	}
	// check otp
	OTPConfig config = otp_read_header();
	if (!config.has_header) {
		error("No header");
	} else {
		print_usb_str("Random:");
		if (config.randomization_finished) {
			print_usb_str("Done\n");
		} else {
			if (config.randomization_started) {
				print_usb_str("Incomplete\n");
			} else {
				print_usb_str("Not randomized\n");
			}
		}
		print_usb_str("Blocks:");
		print_usb_dec(config.block_count);
		print_usb_str("\n");
	}
	print_usb_str("---END DIAGNOSTICS---\n");
}

void scan_bb() {
	uint16_t bbl[10];
	otp_scan_bad_blocks(bbl, 10);
	uint8_t bbl_idx;
	for (bbl_idx = 0; bbl_idx < 10; bbl_idx++) {
		uint8_t buf[10];
		char* tmpl = "BB:      \n";
		uint8_t bufidx = 0;
		if (bbl[bbl_idx] == 0xffff) { break; }
		for (bufidx = 0; bufidx < 10; bufidx++) {
			buf[bufidx] = (uint8_t)tmpl[bufidx];
		}
		uint16_t n = bbl[bbl_idx];
		bufidx = 8;
		while (n != 0) {
			buf[bufidx--] = hex(n&0x0f);
			n >>= 4;
		}
		cdcSendDataWaitTilDone((BYTE*)buf, 10, CDC0_INTFNUM, 100);
	}
}

void scan_used() {
	uint16_t block;
	for (block = 1; block < 2048; block++) {
		uint8_t r = otp_get_block_status(block);
		if (r != BU_UNUSED_BLOCK) {
			print_usb_str("USED ");
			print_usb_dec(block);
			print_usb_str("\n");
		}
	}
}

void read_rng() {
	hwrng_start();
	while (!hwrng_done());
	volatile uint16_t* bits = hwrng_bits();
	cdcSendDataWaitTilDone((BYTE*)bits, 16*4, CDC0_INTFNUM, 100);
}

uint16_t parseDec(uint8_t* buf, uint8_t* idx, uint8_t len) {
	uint16_t val = 0;
	for (; (*idx < len) && (buf[*idx] >= '0') && (buf[*idx] <= '9'); (*idx)++) {
		val = val * 10 + (buf[*idx] - '0');
	}
	return val;
}
// Read paragraph. Parameters are a comma separated list: block, page, paragraph.
bool parseBPP(uint8_t* buf, uint8_t* idx, uint8_t len, uint16_t* block, uint8_t* page, uint8_t* para) {
	*block = parseDec(buf,idx,len);
	if (buf[*idx] != ',') return false; (*idx)++;
	*page = parseDec(buf,idx,len);
	if (buf[*idx] != ',') return false; (*idx)++;
	*para = parseDec(buf,idx,len);
	return true;
}


/**
 * All commands are terminated by a newline character.
 * Standard command summary:
 *
 * D                       - diagnostics
 * #                       - produce 64 bytes of random data from the RNG
 * Rblock,page,para,count  - retrieve (and zero) count paragraphs starting at block,page,para.
 *                           maximum count is 4. will wait for user button press before continuing.
 * Pcount                  - provision (and zero) count paragraphs. snap-pad chooses next available paras.
 *                           maximum count is 4. will wait for user button press before continuing.
 *
 * Additional debug build commands:
 * C                        - print the bad block list
 * U                        - print the used block list
 * cblock                   - print the checksum of the indicated block
 * Mblock                   - mark the given block as used
 * F                        - find the address of the next block containing provisionable paras
 * rblock,page,para         - read the given block, page, and paragraph without erasing
 * Eblock                   - erase the indicated block (0xff everywhere)
 */

void do_usb_command(uint8_t* cmdbuf, uint16_t len) {
	if (cmdbuf[0] == '\n' || cmdbuf[0] == '\r') {
		// skip
	} else if (cmdbuf[0] == 'D') {
		// run diagnostics
		diagnostics();
	} else if (cmdbuf[0] == 'P') {
		// provision 'count' blocks.
		uint8_t idx = 1;
		uint16_t count = parseDec(cmdbuf,&idx,len);
		if (count == 0 || count > 4) {
			error("bad count");
			return;
		}
		if (confirm_count(count)) {
			leds_set_mode(LM_ACKNOWLEDGED);
			otp_provision(count,config.is_A);
			leds_set_mode(LM_READY);
		} else {
			timeout();
		}
	} else if (cmdbuf[0] == 'R') {
		// retrieve 'count' blocks starting at 'block','page','para'
		uint8_t idx = 1;
		struct {
			uint16_t block;
			uint8_t page;
			uint8_t para;
		} bpp[4];
		uint8_t count = 0;
		uint8_t i;
		// parse format: block# "," page# "," para# ["," block# "," page# "," para#]*
		while (true) {
			if (!parseBPP(cmdbuf, &idx, len, &bpp[count].block, &bpp[count].page, &bpp[count].para)) {
				error("PARSE");
				return;
			}
			// validate
			if (bpp[count].block == 0 ||
					bpp[count].block > 2047 ||
					bpp[count].page > 63 ||
					bpp[count].para > 3) {
				error("RANGE");
				return;
			}
			count++;
			if (count == 4) break;
			if (cmdbuf[idx++] != ',') break;
		}
		if (confirm_count(count)) {
			leds_set_mode(LM_ACKNOWLEDGED);
			for (i = 0; i < count; i++) {
				otp_retrieve(bpp[i].block,bpp[i].page,bpp[i].para);
			}
			leds_set_mode(LM_READY);
		} else {
			timeout();
		}
	} else if (cmdbuf[0] == '#') {
		read_rng();
#ifdef DEBUG
	} else if (cmdbuf[0] == 'C') {
		scan_bb();
	} else if (cmdbuf[0] == 'U') {
		scan_used();
	} else if (cmdbuf[0] == 'c') {
		// compute checksum of block
		uint8_t idx = 1;
		uint16_t block = parseDec(cmdbuf,&idx,len);
		struct checksum_ret sum;
		print_usb_str("Starting checksum\n");
		sum = nand_block_checksum(block);
		if (sum.ok) {
			print_usb_str("Finished checksum ");
			print_usb_dec(sum.checksum);
			print_usb_str("\n");
		} else {
			print_usb_str("Bad checksum/read\n");
		}
	} else if (cmdbuf[0] == 'M') {
		// mark block
		uint8_t idx = 1;
		uint16_t block = parseDec(cmdbuf,&idx,len);
		otp_mark_block(block,BU_USED_BLOCK);
		print_usb_str("Marked block ");
		print_usb_dec(block);
		print_usb_str("\n");
	} else if (cmdbuf[0] == 'F') {
		// find next available block
		uint16_t block = otp_find_unmarked_block(!config.is_A);
		print_usb_str("Next unused block ");
		print_usb_dec(block);
		print_usb_str("\n");
	} else if (cmdbuf[0] == 'r') {
		// Read paragraph. Parameters are a comma separated list of decimal values: block, page, paragraph.
		uint16_t block = 0; uint8_t page = 0; uint8_t para = 0;
		uint8_t idx = 1;
		if (parseBPP(cmdbuf,&idx,len,&block,&page,&para)) {
			//usb_debug_dec(block); usb_debug(":");
			//usb_debug_dec(page); usb_debug(":");
			//usb_debug_dec(para); usb_debug("\n");
			if (nand_load_para(block,page,para)) {
				cdcSendDataWaitTilDone((BYTE*)buffers_get_nand(),512,CDC0_INTFNUM,100);
			} else {
				error("READ");
			}
		} else {
			error("PARSE");
		}
	} else if (cmdbuf[0] == 'E') {
		// Erase block. Parameter is a decimal block number.
		uint8_t idx = 1;
		uint16_t block = parseDec(cmdbuf, &idx, len);
		nand_block_erase(block);
		cdcSendDataWaitTilDone((BYTE*)"OK\n", 3, CDC0_INTFNUM, 100);
#endif
	} else {
		cmdbuf[len] = '\0';
		error((const char*)cmdbuf);
		return;
	}
}

#define CMDBUFSZ 128
char cmdbuf[CMDBUFSZ];
uint16_t cmdidx = 0;

bool process_usb() {
	bool processed = false;
	uint16_t delta = cdcReceiveDataInBuffer((BYTE*)(cmdbuf + cmdidx), CMDBUFSZ-cmdidx, CDC0_INTFNUM);
	cmdidx += delta;
	// Scan for eol
	uint16_t c;
	for (c=0;c<cmdidx;c++) {
		char ch = cmdbuf[c];
		if (ch == '\r' || ch == '\n') {
			do_usb_command((uint8_t*)cmdbuf,c);
			processed = true;
			c++;
			uint16_t cb = 0;
			while (c < cmdidx) {
				cmdbuf[cb++] = cmdbuf[c++];
			}
			cmdidx = cb;
			break;
		}
	}
	if (cmdidx == CMDBUFSZ) { cmdidx--; }
	return processed;
}

/*  
 * ======== UNMI_ISR ========
 */
#pragma vector = UNMI_VECTOR
__interrupt VOID UNMI_ISR (VOID)
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;
        case SYSUNIV_NMIIFG:
            __no_operation();
            break;
        case SYSUNIV_OFIFG:
            UCS_clearAllOscFlagsWithTimeout(UCS_BASE, 0);
            SFR_clearInterrupt(SFR_BASE, SFR_OSCILLATOR_FAULT_INTERRUPT);
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;
        case SYSUNIV_BUSIFG:
            // If the CPU accesses USB memory while the USB module is 
            // suspended, a "bus error" can occur.  This generates an NMI, and
            // execution enters this case.  This should never occur.  If USB is
            // automatically disconnecting in your software, set a breakpoint
            // here and see if execution hits it.  See the Programmer's
            // Guide for more information. 
            SYSBERRIV = 0; // Clear bus error flag
            USB_disable(); // Disable USB -- USB must be reset after a bus error
    }
}
//Released_Version_4_00_02

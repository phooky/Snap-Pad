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

void do_factory_test_mode();
void do_factory_uart_mode();
void ok();
void error();

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

    //bool button_pressed_on_startup = has_confirm();

    bool usb_attached = false;
    timer_reset();
    while (timer_msec() < 600) {
    	uint8_t s = USB_connectionState();
        if (s == ST_ENUM_ACTIVE || s == ST_ENUM_SUSPENDED)  {
    		usb_attached = true;
    		break;
    	}
    }
    if (usb_attached) {
    	do_factory_test_mode();
    } else {
    	do_factory_uart_mode();
    }
}

void do_factory_test_mode() {
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

bool checkStr(const char* expected, const char* v) {
	while (*expected != '\0') {
		if (*expected != *v) return false;
		expected++;
		v++;
	}
	return true;
}

void do_factory_uart_mode() {
	char buf[10];
	uint8_t idx = 0;
    while (1)  // main loop
    {
    	uint8_t data = uart_consume();
    	buf[idx++] = data;
    	if (data == '\n') {
    		if (checkStr("PING\n",buf)) {
    			uart_send_buffer("RESP\n",5);
    		} else {
    			uart_send_buffer("FAIL\n",5);
    		}
    	}
    }
}

char hex(uint8_t v) {
	v &= 0x0f;
	if (v < 10) return '0'+v;
	return 'a'+(v-10);
}

// also used to de-decimal
uint8_t dehex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
	if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
	return 0;
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
		error();//"ONFI failure");
		return;
	}
	// check otp
	OTPConfig config = otp_read_header();
	if (!config.has_header) {
		error();//"No header");
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


/**
 * All commands are terminated by a newline character.
 * Factory test command summary:
 *
 * P        - run UART ping test; returns response message as received from far side of board ("RESPONSE") followed by newline
 * Lx       - set LEDs according to hex digit X (LED 1 == LSB, LED 4 == 0x08); returns "OK\n"
 * B        - return button press count within next 5 seconds; returns # of presses in decimal followed by newline
 * #        - produce 64 bytes of random data from the RNG
 * NO       - test the ONFI interface on the NAND chip, return "OK\n" or "ERROR: FAILED\n"
 * NI       - return the ID information of the NAND chip as a comma-separated list terminated with a newline
 * NT       - write, test, and erase a block on the NAND chip, return "OK\n" or "ERROR: FAILED\n"
 * R        - write an entire block of RNG data to the NAND chip, return it to the server as a newline-terminated sequence of hex digits
 * V        - return a string describing the version of the factory test firmware
 *
 */

void ok() {
	print_usb_str("OK\n");
}

void error() {
	print_usb_str("FAILED\n");
}

void do_usb_command(uint8_t* cmdbuf, uint16_t len) {
	if (cmdbuf[0] == '\n' || cmdbuf[0] == '\r') {
		// skip
	} else if (cmdbuf[0] == 'V') {
		// return version number
		print_usb_dec(MAJOR_VERSION); print_usb_str("."); print_usb_dec(MINOR_VERSION); print_usb_str("\n");
	} else if (cmdbuf[0] == 'L') {
		// turn on LEDs
		uint8_t leds = dehex(cmdbuf[1]);
		uint8_t i;
		for (i = 0; i < 4; i++) {
			leds_set_led(i, ((leds & (1<<i)) == 0)?LED_OFF:LED_ON);
		}
		ok();
	} else if (cmdbuf[0] == 'P') {
		char buf[10];
		uint8_t idx = 0;
		char data = 0;
		uart_send_buffer("PING\n",5);
		while (data != '\n' && idx < 9) {
			data = uart_consume_timeout(100);
    		buf[idx++] = data;
		}
    	if (data == '\n' && checkStr("RESP\n",buf)) {
    		ok();
    	} else {
    		buf[idx] = '\0';
    		print_usb_dec(idx);
    		print_usb_str(":");
    		print_usb_str(buf);
    		error();
    	}
	} else if (cmdbuf[0] == 'B') {
		// * B        - return button press count within next 5 seconds; returns # of presses in decimal followed by newline
		uint8_t presses = 0;
		bool timeout = false;
		timer_reset();
		while (!timeout) {
			while (!has_confirm()) { if (timer_msec() >= 5000) { timeout = true; break; } }
			while (has_confirm()) { if (timer_msec() >= 5000) { timeout = true; break; } }
			if (!timeout) presses++;
		}
		print_usb_dec(presses);
		print_usb_str("\n");
	} else if (cmdbuf[0] == '#') {
		// * #        - produce 64 bytes of random data from the RNG
		read_rng();
		ok();
	} else if (cmdbuf[0] == 'N') {
		if (cmdbuf[1] == 'O') {
			// * NO       - test the ONFI interface on the NAND chip, return "OK\n" or "ERROR: FAILED\n"
			if (nand_check_ONFI()) {
				ok();
			} else {
				error();
			}
		} else if (cmdbuf[1] == 'I') {
			// * NI       - return the ID information of the NAND chip as a comma-separated list terminated with a newline
		} else if (cmdbuf[1] == 'T') {
			// * NT       - write, test, and erase a block on the NAND chip, return "OK\n" or "ERROR: FAILED\n"
		} else { error(); return; }
	} else if (cmdbuf[0] == 'R') {
		// * R        - write an entire block of RNG data to the NAND chip, return it to the server as a newline-terminated sequence of hex digits
	} else {
		error();
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

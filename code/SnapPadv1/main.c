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

void run_snap_pad();

ConnectionState cs;

void main (void)
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
    hwrng_init();            // Initialize HW RNG
    initClocks(8000000);     // Configure clocks
    USB_setup(TRUE,TRUE);    // Init USB & events; if a host is present, connect
    uarts_init();			 // Initialize uarts between hosts
	leds_set(0x01);

    __enable_interrupt();    // Enable interrupts globally

    // Work out UART contention
	OTPConfig config = otp_read_header();
	bool master = config.has_header && config.is_A;
	cs = uarts_determine_state(master);

    while (1)  // main loop
    {
        switch(USB_connectionState())
        {
            case ST_ENUM_ACTIVE:
            	run_snap_pad();
            	break;
            case ST_USB_DISCONNECTED: // physically disconnected from the host
            case ST_ENUM_SUSPENDED:   // connecte d/enumerated, but suspended
            case ST_NOENUM_SUSPENDED: // connected, enum started, but the host is unresponsive
                // In this example, for all of these states we enter LPM3.  If 
                // the host performs a "USB resume" from suspend, the CPU will
                // automatically wake.  Other events can also wake the
                // CPU, if their event handlers in eventHandlers.c are 
                // configured to return TRUE.
                //__bis_SR_register(LPM3_bits + GIE);
            	//leds_set(0x09);
                break;

            // The default is executed for the momentary state
            // ST_ENUM_IN_PROGRESS.  Almost always, this state lasts no more than a 
            // few seconds.  Be sure not to enter LPM3 in this state; USB 
            // communication is taking place, so mode must be LPM0 or active.
            case ST_ENUM_IN_PROGRESS:
            	//leds_set(0x06);
            default:;
        }
    }
}

char hex(uint8_t v) {
	v &= 0x0f;
	if (v < 10) return '0'+v;
	return 'a'+(v-10);
}

uint8_t dehex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
	if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
	return 0;
}

void error() {
	char* rsp = "ERR\r\n";
	cdcSendDataWaitTilDone((BYTE*)rsp, 5, CDC0_INTFNUM, 100);
}

void read_status() {
	char sbuf[8];
	uint8_t status = nand_read_status_reg();
	uint8_t i;
	for (i = 0; i < 8; i++) {
		sbuf[7-i] = ((status&0x01)==0)?'0':'1';
	}
	cdcSendDataWaitTilDone((BYTE*)sbuf, 8, CDC0_INTFNUM, 100);
	char* rsp = "\r\nOK\r\n";
	cdcSendDataWaitTilDone((BYTE*)rsp, 6, CDC0_INTFNUM, 100);
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

void read_rng() {
	hwrng_start();
	while (!hwrng_done());
	volatile uint32_t* bits = hwrng_bits();
	cdcSendDataWaitTilDone((BYTE*)bits, 16*4, CDC0_INTFNUM, 100);
}

void debug_dec(int i) {
	char buf[10];
	int ip = i/10;
	int digits = 1;
	while (ip != 0) {
		digits++; ip = ip/10;
	}
	int idx = digits;
	buf[--idx] = '0'+(i%10);
	i = i/10;
	while(i != 0) {
		buf[--idx] = '0'+(i%10);
		i = i/10;
	}
	cdcSendDataWaitTilDone((BYTE*)buf, digits, CDC0_INTFNUM, 100);
}

void debug(char* s) {
	int len = 0;
	while (s[len] != '\0') {
		len++;
		if (len == 64) break;
	}
	cdcSendDataWaitTilDone((BYTE*)s, len, CDC0_INTFNUM, 100);
}

void do_command(uint8_t* cmdbuf, uint16_t len) {
	if (cmdbuf[0] == '\n' || cmdbuf[0] == '\r') {
		// skip
	} else if (cmdbuf[0] == 'T') {
		// check otp
		OTPConfig config = otp_read_header();
		if (!config.has_header) {
			debug("No header\n");
		} else {
			debug("Has header\n");
			if (config.block_map_written) {
				debug("Has block map\n");
				debug("Block count ");
				debug_dec(config.block_count);
				debug("\n");
			}
			if (config.is_A) {
				debug("Is pad A\n");
			}
		}
	} else if (cmdbuf[0] == 'U') {
		// get uart state
		debug("Uart state ");
		if (cs == CS_CONNECTED_MASTER) debug("CONN_MSTR");
		if (cs == CS_CONNECTED_SLAVE) debug("CONN_SLAVE");
		if (cs == CS_NOT_CONNECTED) debug("NO_CONN");
		if (cs == CS_INDETERMINATE) debug("CONN_IND");
		debug("\n");
	} else if (cmdbuf[0] == 'I') {
		// Initialize nand
		otp_initialize_header();
	} else if (cmdbuf[0] == 'l') { // set LEDs
		leds_set(dehex(cmdbuf[1]));
	} else if (cmdbuf[0] == '#') {
		read_rng();
	} else if (cmdbuf[0] == 'C') {
		scan_bb();
	} else if (cmdbuf[0] == 'R' || cmdbuf[0] == 'W' || cmdbuf[0] == 'E') {
		if (cmdbuf[1] != ':') { error(); return; }
		uint32_t addr = 0;
		uint8_t idx = 2;
		while ((idx < len) && (cmdbuf[idx] != ':')) {
			addr = (addr << 4) | dehex(cmdbuf[idx]);
			idx++;
		}
		if (cmdbuf[idx] != ':') { error(); return; }
		idx++;
		if (cmdbuf[0] == 'R') {
			uint16_t rlen = 0;
			while ((idx < len) && (cmdbuf[idx] != '\n' && cmdbuf[idx] != '\r')) {
				rlen = (rlen << 4) | dehex(cmdbuf[idx]);
				idx++;
			}

			nand_read_raw_page(addr, (uint8_t*)cmdbuf, rlen);
			cdcSendDataWaitTilDone((BYTE*)cmdbuf, rlen, CDC0_INTFNUM, 100);
		} else if (cmdbuf[0] == 'E') {
			char* rsp = "OK\r\n";
			nand_block_erase(addr);
			cdcSendDataWaitTilDone((BYTE*)rsp, 4, CDC0_INTFNUM, 100);
		} else {
			char* rsp = "OK\r\n";
			char b[8];
			uint32_t aw = len-idx;
			int8_t bi;
			for (bi = 7; bi >= 0; bi--) {
				b[bi] = hex(aw & 0x0f);
				aw >>= 4;
			}
			cdcSendDataWaitTilDone((BYTE*)b, 8, CDC0_INTFNUM, 100);
			nand_program_raw_page(addr, (uint8_t*)(cmdbuf + idx), len - idx);
			cdcSendDataWaitTilDone((BYTE*)rsp, 4, CDC0_INTFNUM, 100);
		}
	} else {
		cdcSendDataWaitTilDone((BYTE*)cmdbuf, 1, CDC0_INTFNUM, 100);
		error(); return;
	}
}

#define CMDBUFSZ 128
char cmdbuf[CMDBUFSZ];
uint16_t cmdidx = 0;

void run_snap_pad() {
	//leds_set(has_confirm()?0xff:0x00);
	uint16_t delta = cdcReceiveDataInBuffer((BYTE*)(cmdbuf + cmdidx), CMDBUFSZ-cmdidx, CDC0_INTFNUM);
	cmdidx += delta;
	// Scan for eol
	uint16_t c;
	for (c=0;c<cmdidx;c++) {
		char ch = cmdbuf[c];
		if (ch == '\r' || ch == '\n') {
			do_command((uint8_t*)cmdbuf,c);
			c++;
			uint16_t cb = 0;
			while (c < cmdidx) {
				cmdbuf[cb++] = cmdbuf[c++];
			}
			cmdidx = cb;
			break;
		} else {
			c++;
		}
	}
	if (cmdidx == CMDBUFSZ) { cmdidx--; }
	//USBCDC_sendData((BYTE*)&l,	1, CDC0_INTFNUM);
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

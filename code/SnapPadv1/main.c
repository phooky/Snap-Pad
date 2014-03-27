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
#include "timer.h"
#include "buffers.h"

// instantiate connection_state
uint8_t connection_state;

void process_usb();

ConnectionState cs;

void do_factory_reset_mode();
void do_twinned_master_mode();
void do_twinned_slave_mode();
void do_single_mode();

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
    timer_init();			 // Set up msec timer
    hwrng_init();            // Initialize HW RNG
    initClocks(8000000);     // Configure clocks
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

    cs = uart_determine_state(usb_attached || button_pressed_on_startup);

    if (cs == CS_TWINNED_MASTER) {
    	if (button_pressed_on_startup) {
    		do_factory_reset_mode();
    	} else {
    		do_twinned_master_mode();
    	}
    } else if (cs == CS_TWINNED_SLAVE) {
    	leds_set_led(0,0x55);
    	do_twinned_slave_mode();
    } else if (cs == CS_SINGLE) {
    	leds_set_led(0,0xff);
    	do_single_mode();
    }
}

void do_single_mode() {
	// Go ahead to attract mode
	leds_set_larson();
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
	OTPConfig config = otp_read_header();
	if (!config.has_header) {
		otp_initialize_header();
	}

	// Go ahead to attract mode
	leds_set_larson();
    while (!has_confirm())  // waiting for button press
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
        // ping
        if (uart_ping_button()) break;
    }
    leds_set_led(0,0x0f);
    leds_set_led(1,0xf0);
    leds_set_led(2,0x0f);
    leds_set_led(3,0xf0);
    otp_randomize_boards();
}

void do_twinned_slave_mode() {
	// check otp header
	OTPConfig config = otp_read_header();
	if (!config.has_header) {
		otp_initialize_header();
	}

	// Go ahead to attract mode
	leds_set_led(0,0xf);
	leds_set_led(1,0);
	leds_set_led(2,0);
	leds_set_led(3,0xf);
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

void diagnostics() {
	// Display connection state
	if (cs == CS_TWINNED_MASTER) { usb_debug("CS_MASTER\n"); }
	else if (cs == CS_TWINNED_SLAVE) { usb_debug("CS_SLAVE\n"); }
	else if (cs == CS_SINGLE) { usb_debug("CS_SINGLE\n"); }
	else { usb_debug("CS_?\n"); }
	// Check chip connection
	if (!nand_check_ONFI()) {
		usb_debug("ONFI FAIL\n");
		return;
	}
	// check otp
	OTPConfig config = otp_read_header();
	if (!config.has_header) {
		usb_debug("NOHD\n");
	} else {
		usb_debug("HD\n");
		if (config.block_map_written) {
			usb_debug("BMAP\n");
			usb_debug("BCNT");
			usb_debug_dec(config.block_count);
			usb_debug("\n");
		}
	}

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
	volatile uint16_t* bits = hwrng_bits();
	cdcSendDataWaitTilDone((BYTE*)bits, 16*4, CDC0_INTFNUM, 100);
}

void rand_para_test() {
	uint8_t* p = buffers_get_nand();
	// buffers_get_rng();
	// nand_para_buffer();
	leds_set_led(2,0x55);
	hwrng_bits_start(p,512);
	while (!hwrng_bits_done()) ;
	leds_set_led(2,0x00);
	cdcSendDataWaitTilDone((BYTE*)p,512,CDC0_INTFNUM,100);
}

void usb_debug_dec(int i) {
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

void usb_debug(char* s) {
	int len = 0;
	while (s[len] != '\0') {
		len++;
		if (len == 64) break;
	}
	cdcSendDataWaitTilDone((BYTE*)s, len, CDC0_INTFNUM, 100);
}

void do_usb_command(uint8_t* cmdbuf, uint16_t len) {
	if (cmdbuf[0] == '\n' || cmdbuf[0] == '\r') {
		// skip
	} else if (cmdbuf[0] == 'D') {
		// run diagnostics
		diagnostics();
	} else if (cmdbuf[0] == 'P') {
		// random paragraph test
		rand_para_test();
	} else if (cmdbuf[0] == 'T')  {
		if (cmdbuf[1] == 'r') {
			// time random number production
			uint16_t paras = 4 * 64 * 4;
			usb_debug("start\n");
			while (paras-- > 0) {
				hwrng_bits_start(nand_para_buffer(), 512);
				while(!hwrng_bits_done()) ;
			}
			usb_debug("stop\n");
		} else if (cmdbuf[1] == 'w') {
			uint8_t blocks;
			usb_debug("start\n");
			for (blocks = 0; blocks < 4; blocks++) {
				nand_block_erase(nand_make_addr(0,blocks,0,0));
				uint8_t page;
				for (page = 0; page < 64; page++) {
					uint8_t para;
					for (para = 0; para < 4; para++) {
						nand_save_para(blocks, page, para);
						nand_wait_for_ready();
					}
				}
			}
			usb_debug("stop\n");
		} else if (cmdbuf[1] == 'e') {
			// time serial roundtrip
		}
		// run full production test suite
	} else if (cmdbuf[0] == 'C') {
		scan_bb();
	//} else if (cmdbuf[0] == 'I') {
	// Initialize nand
	//	otp_initialize_header();
	} else if (cmdbuf[0] == '#') {
		read_rng();
#ifdef DEBUG
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
#endif
	} else {
		cdcSendDataWaitTilDone((BYTE*)cmdbuf, 1, CDC0_INTFNUM, 100);
		error(); return;
	}
}

#define CMDBUFSZ 128
char cmdbuf[CMDBUFSZ];
uint16_t cmdidx = 0;

void process_usb() {
	uint16_t delta = cdcReceiveDataInBuffer((BYTE*)(cmdbuf + cmdidx), CMDBUFSZ-cmdidx, CDC0_INTFNUM);
	cmdidx += delta;
	// Scan for eol
	uint16_t c;
	for (c=0;c<cmdidx;c++) {
		char ch = cmdbuf[c];
		if (ch == '\r' || ch == '\n') {
			do_usb_command((uint8_t*)cmdbuf,c);
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

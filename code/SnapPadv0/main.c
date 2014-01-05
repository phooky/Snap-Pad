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

void run_snap_pad();

char hex(uint8_t v) {
	v &= 0x0f;
	if (v < 10) return '0'+v;
	return 'a'+(v-10);
}

#define CMDBUFSZ 128
char cmdbuf[CMDBUFSZ];
uint16_t cmdidx;

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
    nand_init();
    hwrng_init();
    initClocks(8000000);     // Configure clocks
    USB_setup(TRUE,TRUE);    // Init USB & events; if a host is present, connect

    __enable_interrupt();    // Enable interrupts globally

    cmdidx = 0;

    while (1)
    {
        // This switch() creates separate main loops, depending on whether USB 
        // is enumerated and active on the host, or disconnected/suspended.  If
        // you prefer, you can eliminate the switch, and just call
        // USB_connectionState() prior to sending data (to ensure the state is
        // ST_ENUM_ACTIVE). 
        switch(USB_connectionState())
        {
            // This case is executed while your device is connected to the USB
            // host, enumerated, and communication is active.  Never enter 
            // LPM3/4/5 in this mode; the MCU must be active or LPM0 during USB
            // communication.
            case ST_ENUM_ACTIVE:
            	run_snap_pad();
            	break;
            // These cases are executed while your device is:
            case ST_USB_DISCONNECTED: // physically disconnected from the host
            case ST_ENUM_SUSPENDED:   // connected/enumerated, but suspended
            case ST_NOENUM_SUSPENDED: // connected, enum started, but the host is unresponsive
            
                // In this example, for all of these states we enter LPM3.  If 
                // the host performs a "USB resume" from suspend, the CPU will
                // automatically wake.  Other events can also wake the
                // CPU, if their event handlers in eventHandlers.c are 
                // configured to return TRUE.
                __bis_SR_register(LPM3_bits + GIE);  
                break;

            // The default is executed for the momentary state
            // ST_ENUM_IN_PROGRESS.  Almost always, this state lasts no more than a 
            // few seconds.  Be sure not to enter LPM3 in this state; USB 
            // communication is taking place, so mode must be LPM0 or active.
            case ST_ENUM_IN_PROGRESS:
            default:;
        }
    }  //while(1)
} //main()

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

void read_info() {
	char idbuf[11];
	IdInfo id = nand_read_id();
	idbuf[1] = hex(id.manufacturer_code); idbuf[0] = hex(id.manufacturer_code >> 4);
	idbuf[3] = hex(id.device_id); idbuf[2] = hex(id.device_id >> 4);
	idbuf[5] = hex(id.details1); idbuf[4] = hex(id.details1 >> 4);
	idbuf[7] = hex(id.details2); idbuf[6] = hex(id.details2 >> 4);
	idbuf[9] = hex(id.ecc_info); idbuf[8] = hex(id.ecc_info >> 4);
	if (nand_check_ONFI()) {
		idbuf[10] = 'Y';
	} else {
		idbuf[10] = 'N';
	}
	cdcSendDataWaitTilDone((BYTE*)idbuf, 12, CDC0_INTFNUM, 100);
	char* rsp = "\r\nOK\r\n";
	cdcSendDataWaitTilDone((BYTE*)rsp, 6, CDC0_INTFNUM, 100);
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

void read_parameter_pages() {
	uint8_t sbuf[256];
	nand_read_parameter_page(sbuf, 256);
	cdcSendDataWaitTilDone((BYTE*)sbuf, 256, CDC0_INTFNUM, 100);
}

void read_hack(uint8_t* buffer){
	uint8_t val = (dehex(buffer[1]) << 4) | dehex(buffer[2]);
	char cmd = (char)buffer[0];
	if (cmd == 'C') {
		nand_send_command(val);
	} else if (cmd == 'A') {
		nand_send_address(val);
	} else if (cmd == 'R') {
		uint8_t readbuf[256];
		nand_recv_data(readbuf, val);
		cdcSendDataWaitTilDone((BYTE*)readbuf, val, CDC0_INTFNUM, 100);
	}
}

void soft_boot() {
	// msp430 reboot-- can we drop back to the BSL?
}

void do_command(uint16_t len) {
	if (cmdbuf[0] == 'I') { // reading chip info structure
		read_info();
	} else if (cmdbuf[0] == 'S') { // nand status
		read_status();
	} else if (cmdbuf[0] == 'H') { // start hack mode
		read_hack((uint8_t*)cmdbuf+1);
	} else if (cmdbuf[0] == 'B') { // reboot msp430
		soft_boot();
	} else if (cmdbuf[0] == 'P') { // read and dump parameter pages
		read_parameter_pages();
	} else if (cmdbuf[0] == '+') { // turn on hardware trng power
		hwrng_power(true);
		char* rsp = "OK\r\n";
		cdcSendDataWaitTilDone((BYTE*)rsp, 4, CDC0_INTFNUM, 100);
	} else if (cmdbuf[0] == '-') { // turn off hardware trng power
		hwrng_power(false);
		char* rsp = "OK\r\n";
		cdcSendDataWaitTilDone((BYTE*)rsp, 4, CDC0_INTFNUM, 100);
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

			char b[8];
			int8_t bi;
			for (bi = 0; bi < 8; bi++) {
				b[bi] = hex(cmdbuf[bi]);
			}
			cdcSendDataWaitTilDone((BYTE*)b, 8, CDC0_INTFNUM, 100);
			char* rsp = "[";
			cdcSendDataWaitTilDone((BYTE*)rsp, 1, CDC0_INTFNUM, 100);

			cdcSendDataWaitTilDone((BYTE*)cmdbuf, rlen, CDC0_INTFNUM, 100);
			rsp = "]\r\n";
			cdcSendDataWaitTilDone((BYTE*)rsp, 3, CDC0_INTFNUM, 100);
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
	} else { error(); return; }
}

void run_snap_pad() {
	uint16_t delta = cdcReceiveDataInBuffer((BYTE*)(cmdbuf + cmdidx), CMDBUFSZ-cmdidx, CDC0_INTFNUM);
	uint16_t c;
	for (c = cmdidx; c < cmdidx+delta; c++) {
		char ch = cmdbuf[c];
		if (ch == '\r' || ch == '\n') {
			do_command(c);
			cmdidx = 0;
			return;
		}
	}
	cmdidx += delta;
	if (cmdidx == CMDBUFSZ) { cmdidx = 0; }
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

/*
 * print.c
 *
 *  Created on: Apr 29, 2014
 *      Author: phooky
 */

#include "print.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/types.h"
#include "USB_API/USB_Common/usb.h"
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"
#include "USB_config/descriptors.h"
#include "base64.h"

void print_usb_dec(uint32_t i) {
	char buf[10];
	unsigned int digits = 0;
	do {
		digits++;
		buf[10-digits] = '0' + (i % 10);
		i = i / 10;
	} while(i);
	cdcSendDataWaitTilDone((BYTE*)buf+(10-digits), digits, CDC0_INTFNUM, 100);
}

char hex(uint8_t v) {
	v &= 0x0f;
	if (v < 10) return '0'+v;
	return 'a'+(v-10);
}

// Print 1B hex digit to USB serial port
void print_usb_hex(const uint8_t i) {
	char buf[2];
	buf[0] = hex(i>>4);
	buf[1] = hex(i);
	cdcSendDataWaitTilDone((BYTE*)buf, 2, CDC0_INTFNUM, 100);
}

void print_usb_str(const char* s) {
	int len = 0;
	while (s[len] != '\0') {
		len++;
		if (len == 64)
			break;
	}
	cdcSendDataWaitTilDone((BYTE*) s, len, CDC0_INTFNUM, 100);
}

// Incremental base64 printer
uint8_t in[3];
uint8_t out[4];
uint8_t line;
uint8_t count;

void b64_print_init() {
	line = 0; count = 0;
}

void b64_print_buffer(uint8_t* buf, uint16_t sz) {
	while (sz > 0) {
		while ((count < 3) && (sz>0)) {
			in[count++] = *(buf++);
			sz--;
		}
		if (count == 3) {
			encode(in,out,3);
			cdcSendDataWaitTilDone((BYTE*) out, 4, CDC0_INTFNUM, 100);
			line += 4;
			if (line > 76) {
				cdcSendDataWaitTilDone((BYTE*) "\n", 1, CDC0_INTFNUM, 100);
				line = 0;
			}
			count = 0;
		}
	}
}

void b64_print_finish() {
	if (count > 0) {
		int l = count;
		while (l < 3) { in[l++] = 0; }
	}
	encode(in,out,count);
	cdcSendDataWaitTilDone((BYTE*) out, 4, CDC0_INTFNUM, 100);
}

void print_usb_base64(uint8_t* buf, uint16_t sz) {
	uint8_t in[3];
	uint8_t out[4];
	uint8_t line = 0;
	uint8_t i;
	while (sz > 0) {
		uint8_t l = (sz > 3) ? 3 : sz;
		for (i = 0; i < 3; i++) {
			if (sz > 0) {
				in[i] = *(buf++);
				sz--;
			} else {
				in[i] = 0;
			}
		}
		encode(in, out, l);
		cdcSendDataWaitTilDone((BYTE*) out, 4, CDC0_INTFNUM, 100);
		line += 4;
		if (line > 76) {
			cdcSendDataWaitTilDone((BYTE*) "\n", 1, CDC0_INTFNUM, 100);
			line = 0;
		}
	}
}

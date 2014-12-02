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

void print_usb_dec(unsigned int i) {
	char buf[10];
	unsigned int ip = i/10;
	unsigned int digits = 1;
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

void print_usb_str(const char* s) {
	int len = 0;
	while (s[len] != '\0') {
		len++;
		if (len == 64) break;
	}
	cdcSendDataWaitTilDone((BYTE*)s, len, CDC0_INTFNUM, 100);
}

void print_usb_base64(uint8_t* buf, uint16_t sz) {
	uint8_t in[3];
	uint8_t out[4];
	uint8_t line = 0;
	uint8_t i;
	while (sz > 0) {
		uint8_t l = (sz>3)?3:sz;
		for (i = 0; i < 3; i++) {
			if (sz > 0) {
				in[i] = *(buf++);
				sz--;
			} else {
				in[i] = 0;
			}
		}
		encode(in, out, l);
		cdcSendDataWaitTilDone((BYTE*)out, 4, CDC0_INTFNUM, 100);
		line += 4;
		if (line > 76) {
			cdcSendDataWaitTilDone((BYTE*)"\n", 1, CDC0_INTFNUM, 100);
			line = 0;
		}
	}
}

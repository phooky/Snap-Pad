/*
 * print.h
 *
 *  Created on: Apr 29, 2014
 *      Author: phooky
 */

#ifndef PRINT_H_
#define PRINT_H_

#include <stdint.h>

// Print integer to USB serial port
void print_usb_dec(uint32_t i);

// Hex conversion convenience fn
char hex(uint8_t v);

// Print 1B hex digit to USB serial port
void print_usb_hex(const uint8_t i);

// Print null-terminated string to USB serial port
void print_usb_str(const char* s);

// Print base64 encoding of passed buffer
void print_usb_base64(uint8_t* buf, uint16_t sz);

// Stateful printing of b64 data
void b64_print_init(); // start new print encode
void b64_print_buffer(uint8_t* buf, uint16_t sz);
void b64_print_finish();

#endif /* PRINT_H_ */

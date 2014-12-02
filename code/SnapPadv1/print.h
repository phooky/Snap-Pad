/*
 * print.h
 *
 *  Created on: Apr 29, 2014
 *      Author: phooky
 */

#ifndef PRINT_H_
#define PRINT_H_

#include <stdint.h>

void print_usb_dec(unsigned int i);
void print_usb_str(const char* s);
void print_usb_base64(uint8_t* buf, uint16_t sz);

#endif /* PRINT_H_ */

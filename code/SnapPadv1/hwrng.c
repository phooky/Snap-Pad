/*
 * hwrng.c
 *
 *  Created on: Dec 24, 2013
 *      Author: phooky
 */

#include "hwrng.h"
#include <msp430f5508.h>
#include <stdint.h>

// Analog pin 0 is P6.0
#define A_PIN BIT0

void hwrng_init() {
	P6DIR |= A_PIN;
	P6SEL |= A_PIN;

	// Set up ADC
	ADC10CTL0 = 0x0090;
	ADC10CTL1 = 0x020C;
	ADC10CTL2 = 0x0010;
	ADC10IE = 0x0001;
}

volatile uint16_t bits[8];
volatile uint8_t idx = 0;
volatile uint8_t* hwrng_ptr = 0;
volatile uint16_t hwrng_len = 0;

void hwrng_start() {
	idx = 0;
	ADC10CTL0 |= 0x0003;
}

bool hwrng_done() {
	return idx >= 128;
}

volatile uint16_t* hwrng_bits() {
	return bits;
}

void hwrng_bits_start(uint8_t* ptr, uint16_t len) {
	hwrng_ptr = ptr;
	hwrng_len = len;
	hwrng_start();
}

bool hwrng_bits_done() {
	return hwrng_done();
}


#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	uint16_t s = ADC10MEM0;

	uint16_t v = bits[idx>>4];
	v = v<<2 | v>>14;
	v ^= s;
	bits[idx>>4] = v;
	idx++;

	if (idx >= 128) {
		if (hwrng_len > 0) {
			uint8_t i = 0;
			uint8_t* p = (uint8_t*)bits;
			// pack bits into ptr
			while( i < 16 && hwrng_len > 0) {
				*(hwrng_ptr++) = p[i++];
				hwrng_len--;
			}
		}
		if (hwrng_len == 0) {
			ADC10CTL0 &= ~0x0003;
		} else {
			idx = 0;
		}
	}
}

/*
 * hwrng.c
 *
 *  Created on: Dec 24, 2013
 *      Author: phooky
 */

#include "hwrng.h"
#include <msp430f5508.h>
#include <stdint.h>
//
// Port 6 Pin 1 is the power pin
//

#define SHDWN_P_PIN BIT1

// Analog pin 0 is P6.0
#define A_PIN BIT0

void hwrng_init() {
	hwrng_power(false);
	P6DIR |= SHDWN_P_PIN;
	P6SEL &= ~SHDWN_P_PIN;

	P6DIR |= A_PIN;
	P6SEL |= A_PIN;

	// Set up ADC
	ADC10CTL0 = 0x0090;
	ADC10CTL1 = 0x020C;
	ADC10CTL2 = 0x0010;
	ADC10IE = 0x0001;
}

void hwrng_power(bool on) {
	if (on) {
		P6OUT |= SHDWN_P_PIN;
	} else {
		P6OUT &= ~SHDWN_P_PIN;
	}
}

volatile uint32_t bits[16];
volatile uint16_t idx = 0;

void hwrng_start() {
	idx = 0;
	ADC10CTL0 |= 0x0003;
}

bool hwrng_done() {
	return idx >= 256;
}

volatile uint32_t* hwrng_bits() {
	return bits;
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	uint32_t s = ADC10MEM0;
	if (idx >= 256) {
		ADC10CTL0 &= ~0x0003;
	} else {
		uint32_t v = bits[idx>>4];
		v = v<<2 | v>>30;
		v ^= s;
		bits[idx>>4] = v;
		idx++;
	}
}

/*
 * leds.c
 *
 *  Created on: Jan 25, 2014
 *      Author: phooky
 */

#include "leds.h"
#include "msp430.h"


/**
 * Pin assignments
 *
 * LED0 - P5.0
 * LED1 - P5.1
 * LED2 - P5.4
 * LED3 - P5.5
 */

#define ALL_LED_BITS (1<<0 | 1<<1 | 1<<4 | 1<<5)
/**
 * Initialize LED pins
 */
void leds_init() {
	P5OUT |= ALL_LED_BITS;
	P5DIR |= ALL_LED_BITS;
}

/**
 * Turn on and off the four LEDs.
 * @param leds bits 0-3 correspond to LED 0-3.
 */
void leds_set(uint8_t leds) {
	uint8_t out = P5OUT & ~ALL_LED_BITS;
	if ((leds & 1<<0) == 0) out |= 1 << 0;
	if ((leds & 1<<1) == 0) out |= 1 << 1;
	if ((leds & 1<<2) == 0) out |= 1 << 4;
	if ((leds & 1<<3) == 0) out |= 1 << 5;
	P5OUT = out;
}


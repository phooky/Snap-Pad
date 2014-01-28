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
 *
 * CONFIRM - P6.3
 */

#define ALL_LED_BITS (1<<0 | 1<<1 | 1<<4 | 1<<5)
/**
 * Initialize LED pins and switch
 */
void leds_init() {
	P5OUT |= ALL_LED_BITS;
	P5DIR |= ALL_LED_BITS;
	P6DIR &= ~1<<3; // set as input
	P6OUT |= 1<<3;  // pull resistor is high
	P6REN |= 1<<3;  // turn on pull up
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

/**
 * Detect whether the CONFIRM button is currently being held down.
 * @return true if CONFIRM is depressed.
 */
bool has_confirm() {
	return (P6IN & (1<<3)) == 0;
}

/**
 * Wait for a complete depress and release of the confirm button.
 * Handles debouncing. Returns when CONFIRM has been released.
 * TODO: debouncing!!!
 */
void wait_for_confirm() {
	while (has_confirm());
	while (!has_confirm());
	while (has_confirm());
}

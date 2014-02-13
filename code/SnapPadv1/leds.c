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
	// Set up blink timer
	TA0CCR0 = 31250;					  	// Count up to 1/4 second
	TA0CCTL0 = 0x10;					  	// Enable counter interrupts, bit 4=1
	TA0EX0 = 0x07;							// Additional /8
	TA0CTL = TASSEL_1 | MC_1 | ID1 | ID0;	// Clock SMCLK, /8, upcount mode
	TA0CTL |= TACLR;						// Clear and restart clock
}


volatile uint8_t phase = 0;

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0 (void) {
	phase = (phase + 1) % 8;
	leds_set(phase);
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

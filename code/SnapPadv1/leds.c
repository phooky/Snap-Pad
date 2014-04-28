/*
 * leds.c
 *
 *  Created on: Jan 25, 2014
 *      Author: phooky
 */

#include "leds.h"
#include "msp430.h"
#include "timer.h"

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

uint8_t phase = 0;
uint8_t led_mode[LED_COUNT];

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
	TA0CCR0 = 15625;					  	// Count up to 1/8 second
	TA0CCTL0 = 0x10;					  	// Enable counter interrupts, bit 4=1
	TA0EX0 = 0x07;							// Additional /8
	TA0CTL = TASSEL_1 | MC_1 | ID1 | ID0;	// Clock SMCLK, /8, upcount mode
	TA0CTL |= TACLR;						// Clear and restart clock
}

void leds_set_led(uint8_t led, uint8_t mode) {
	led_mode[led] = mode;
}

void leds_set_mode(uint8_t mode) {
	uint8_t i;
	for (i = 0; i < 4; i++) { led_mode[i] = LED_OFF; }
	switch (mode) {
	case LM_OFF:
		break;
	case LM_READY:
		led_mode[0] = LED_ON; break;
	case LM_CONFIRM_4:
		led_mode[3] = LED_SLOW_0;
	case LM_CONFIRM_3:
		led_mode[2] = LED_SLOW_0;
	case LM_CONFIRM_2:
		led_mode[1] = LED_SLOW_0;
	case LM_CONFIRM_1:
		led_mode[0] = LED_SLOW_0;
		break;
	case LM_ACKNOWLEDGED:
		led_mode[0] = LED_ON;
		led_mode[3] = LED_ON;
		break;
	case LM_TIMEOUT:
		led_mode[1] = LED_ON;
		led_mode[2] = LED_ON;
		break;
	case LM_EXHAUSTED:
		for (i = 0; i < 4; i++) { led_mode[i] = LED_FAST_0; }
		break;
	case LM_DUAL_NOT_PROG:
		// Larson scanner mode!
		led_mode[0] = 0x81;
		led_mode[1] = 0x42;
		led_mode[2] = 0x24;
		led_mode[3] = 0x18;
		break;
	case LM_DUAL_PARTIAL_PROG:
		led_mode[0] = led_mode[1] = LED_SLOW_0;
		led_mode[2] = led_mode[3] = LED_SLOW_1;
		break;
	case LM_DUAL_PROG_DONE:
		for (i = 0; i < 4; i++) { led_mode[i] = LED_ON; }
		break;
	}
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

bool confirm_count(uint8_t count) {
	// Before we start, make sure the button is released
	timer_reset();
	while (has_confirm()) {
		if  (timer_msec() >= 1000) { return false; } // you get a second to take your finger off the button
	}
	leds_set_mode(LM_CONFIRM_1 + (count-1));
	timer_reset();
	while (!has_confirm()) {
		if (timer_msec() >= 10000) {
			// Indicate timeout
			leds_set_mode(LM_TIMEOUT);
			timer_reset();
			while (timer_msec() < 1000) {}
			leds_set_mode(LM_READY);
			return false;
		}
	}
	return true;
}

inline bool is_on(uint8_t mode, uint8_t phase) {
	return (mode & (1<<phase)) != 0;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0 (void) {
	phase = (phase + 1) % 8;
	uint8_t out = P5OUT & ~ALL_LED_BITS;
	if (!is_on(led_mode[0],phase)) out |= 1 << 0;
	if (!is_on(led_mode[1],phase)) out |= 1 << 1;
	if (!is_on(led_mode[2],phase)) out |= 1 << 5;
	if (!is_on(led_mode[3],phase)) out |= 1 << 4;
	P5OUT = out;
}

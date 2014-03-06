/*
 * timer.c
 *
 *  Created on: Mar 5, 2014
 *      Author: phooky
 */

#include "timer.h"
#include "msp430.h"

volatile uint16_t msecs = 0;

void timer_init() {
	// Set up msec timer
	TA1CCR0 = 1000;					  	    // Count up to 1ms
	TA1CCTL0 = 0x10;					  	// Enable counter interrupts, bit 4=1
	TA1CTL = TASSEL_1 | MC_1 | ID1 | ID0;	// Clock SMCLK, /8, continuous up mode
	TA1CTL |= TACLR;						// Clear and restart clock
}

void timer_reset() {
	msecs = 0;
	TA1CTL |= TACLR;						// Clear and restart clock
}

uint16_t timer_msec() {
	return msecs;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void) {
	msecs++;
}

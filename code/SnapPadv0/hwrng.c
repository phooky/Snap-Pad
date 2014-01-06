/*
 * hwrng.c
 *
 *  Created on: Dec 24, 2013
 *      Author: phooky
 */

#include "hwrng.h"
#include <msp430f5508.h>

//
// Port 6 Pin 1 is the power pin
//

#define SHDWN_P_PIN BIT1

// Port 1 pin 0 is ACLK output
#define ACLK_PIN BIT0

void hwrng_init() {
	hwrng_power(false);
	P6DIR |= SHDWN_P_PIN;
	P6SEL &= ~SHDWN_P_PIN;
    //P1DIR |= ACLK_PIN;
    //P1SEL |= ACLK_PIN;
}

void hwrng_power(bool on) {
	if (on) {
		P6OUT |= SHDWN_P_PIN;
	} else {
		P6OUT &= ~SHDWN_P_PIN;
	}
}


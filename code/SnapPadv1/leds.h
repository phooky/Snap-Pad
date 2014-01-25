/*
 * leds.h
 *
 *  Created on: Jan 25, 2014
 *      Author: phooky
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>

/**
 * Initialize LED pins
 */
void leds_init();

/**
 * Turn on and off the four LEDs.
 * @param leds bits 0-3 correspond to LED 0-3.
 */
void leds_set(uint8_t leds);


#endif /* LEDS_H_ */

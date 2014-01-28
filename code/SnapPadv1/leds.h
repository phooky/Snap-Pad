/*
 * leds.h
 *
 *  Created on: Jan 25, 2014
 *      Author: phooky
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>
#include <stdbool.h>

/// A bit of a misnomer: LEDs and buttons, really.
/**
 * Initialize LED pins and switch
 */
void leds_init();

/**
 * Turn on and off the four LEDs.
 * @param leds bits 0-3 correspond to LED 0-3.
 */
void leds_set(uint8_t leds);

/**
 * Detect whether the CONFIRM button is currently being held down.
 * @return true if CONFIRM is depressed.
 */
bool has_confirm();

/**
 * Wait for a complete depress and release of the confirm button.
 * Handles debouncing. Returns when CONFIRM has been released.
 */
void wait_for_confirm();

#endif /* LEDS_H_ */

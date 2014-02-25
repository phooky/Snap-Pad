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

/***
 * Calls to handle LED display modes and button presses.
 */

/**
 * Initialize LED pins and switch
 */
void leds_init();

/**
 * Turn on and off the four LEDs.
 * @param leds bits 0-3 correspond to LED 0-3.
 */
void leds_set(uint8_t leds);

#define LED_COUNT 4

enum {
	LED_OFF = 0,
	LED_ON = 0xff,
	LED_SLOW_0 = 0x0f,
	LED_SLOW_1 = 0xf0,
	LED_FAST_0 = 0x33,
	LED_FAST_1 = 0xcc,
	LED_HYPER_0 = 0x55,
	LED_HYPER_1 = 0xaa
};


/**
 * Set the high-level behavior of each led.
 * @param led the index of the LED to set (0-3).
 * @param mode a bitfield representing the phases that the LED should be lit for.
 */
void leds_set_led(uint8_t led, uint8_t mode);

/**
 * Convenience method to set Larson scanner mode.
 */
void leds_set_larson();

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

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

#define LED_COUNT 4

// Individual LED modes
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

// High-level description of LED modes.
enum {
	LM_OFF,
	LM_READY,
	LM_CONFIRM_1,
	LM_CONFIRM_2,
	LM_CONFIRM_3,
	LM_CONFIRM_4,
	LM_ACKNOWLEDGED,
	LM_TIMEOUT,
	LM_EXHAUSTED,

	LM_DUAL_NOT_PROG,
	LM_DUAL_PARTIAL_PROG,
	LM_DUAL_PROG_DONE,

	LM_LAST
};

/**
 * Set the behavior of an individual led.
 * @param led the index of the LED to set (0-3).
 * @param mode a bitfield representing the phases that the LED should be lit for.
 */
void leds_set_led(uint8_t led, uint8_t mode);

/**
 * Convenience method to set high-level LED modes.
 * @param mode State to indicate, see the enum of LM_ modes above
 */
void leds_set_mode(uint8_t mode);

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

/**
 * Give the user ten seconds to confirm releasing blocks. The a number of LEDs
 * flash proportional to the number of blocks to release.
 */
bool confirm_count(uint8_t count);

#endif /* LEDS_H_ */

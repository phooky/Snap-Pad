/*
 * hwrng.h
 *
 *  Created on: Dec 24, 2013
 *      Author: phooky
 */

#ifndef HWRNG_H_
#define HWRNG_H_

#include <stdbool.h>
#include <stdint.h>

void hwrng_start();
bool hwrng_done();

// Return a volatile pointer to the top of the RNG bit buffer.
// It's the caller's responsibility to make sure these bits have
// been properly populated.
volatile uint16_t* hwrng_bits();

// Functions below are intended to be used internally.
/**
 * Write bits to the given buffer.
 * @param ptr start of buffer to write to
 * @param len length of buffer in bytes
 */
void hwrng_bits_start(uint8_t* ptr, uint16_t len);
bool hwrng_bits_done();

void hwrng_init();

#endif /* HWRNG_H_ */

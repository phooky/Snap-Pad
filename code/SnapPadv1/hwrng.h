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
volatile uint16_t* hwrng_bits();

/**
 * Write bits to the given buffer
 */
void hwrng_bits_start(uint8_t* ptr, uint16_t len);
bool hwrng_bits_done();

void hwrng_init();

#endif /* HWRNG_H_ */

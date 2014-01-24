/*
 * ecc.h
 *
 *  Created on: Jan 21, 2014
 *      Author: phooky
 */

#ifndef ECC_H_
#define ECC_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Generate a 25-bit SEC-DED code for a 512 byte buffer.
 * @param buffer the 512B buffer to generate an ECC for
 * @return the ECC
 */
uint32_t ecc_generate(uint8_t* buffer);

/**
 * Attempt to verify and correct a 512 byte buffer with the given ECC. If an error can be
 * corrected, it will be fixed in the passed buffer.
 * @param buffer the 512B buffer to verify
 * @param code the ECC code to verify with
 * @return true if no error or the error was corrected; false if the buffer cannot be fixed.
 */
bool ecc_verify(uint8_t* buffer, uint32_t code);

#endif /* ECC_H_ */

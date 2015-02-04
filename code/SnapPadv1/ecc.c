/*
 * ecc.c
 *
 *  Created on: Jan 21, 2014
 *      Author: phooky
 */

#include "ecc.h"

/**
 *  ECC Code Layout
 *  ---------------------------------------------------------
 *  |   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |
 *  ---------------------------------------------------------
 *  | LP15 | LP13 | LP11 |  LP9 |  LP7 |  LP5 |  LP3 |  LP1 | B0
 *  ---------------------------------------------------------
 *  | LP14 | LP12 | LP10 |  LP8 |  LP6 |  LP4 |  LP2 |  LP0 | B1
 *  ---------------------------------------------------------
 *  |  CP5 |  CP4 |  CP3 |  CP2 |  CP1 |  CP0 | LP17 | LP16 | B2
 *  ---------------------------------------------------------
 *  |  --  |  --  |  --  |  --  |  --  |  --  |  --  |  EXT | B3
 *  ---------------------------------------------------------
 */

/**
 * Compute the parity of the given byte.
 */
static inline uint8_t byte_parity(uint8_t b) {
	b ^= b>>4;
	b ^= b>>2;
	b ^= b>>1;
	return b & 0x01;
}

// attempt at speedup
// just to be clear, I am not proud of this. :_(

static inline uint8_t column_parities(uint8_t b) {
	uint8_t p;
	// compute bits 2 and 3
	uint8_t b23 = b ^ (b >> 4);
	b23 ^= b23 >> 2; // bit 0 -> 2, bit 1 -> 3
	p  = (b23 & 0x3) << 2;
	// compute bits 4 and 5
	uint8_t b45 = b ^ (b >>4);
	b45 ^= b45 >> 1; // bit 0 -> bit 4, bit 2 -> bit 5;
	p |= (b45 & 0x01) << 4;
	p |= (b45 & 0x04) << 3;
	// compute bits 6 and 7
	uint8_t b67 = b ^ (b >> 2);
	b67 ^= b67 >> 1; // bit 0 -> 6, bit 4 -> 7
	p |= (b67 & 0x01) << 6;
	p |= (b67 & 0x10) << 3;
	return p;
}
/**
 * Generate a 25-bit SEC-DED code for a 512 byte buffer.
 * @param buffer the 512B buffer to generate an ECC for
 * @return the ECC
 */
uint32_t ecc_generate(uint8_t* buffer) {
	uint16_t idx;
	uint8_t ecc[4] = {0,0,0,0};
	for (idx = 0;idx < 512;idx++) {
		uint8_t c = buffer[idx];
		// compute line parity
		uint8_t x = c ^ (c>>4);
		x ^= (x>>2);
		x ^= (x>>1);
		x = (x & 0x01)?0xff:0x00;
		ecc[0] ^= idx & x;
		ecc[1] ^= ~idx & x;
        ecc[2] ^= ((idx & 0x100)?0x02:0x01) & x;
		// compute column parities
		#if 1
		ecc[2] ^= column_parities(c);
		#else
		ecc[2] ^= byte_parity(0x55 & c) << 2;
		ecc[2] ^= byte_parity(0xaa & c) << 3;
		ecc[2] ^= byte_parity(0x33 & c) << 4;
		ecc[2] ^= byte_parity(0xcc & c) << 5;
		ecc[2] ^= byte_parity(0x0f & c) << 6;
		ecc[2] ^= byte_parity(0xf0 & c) << 7;
		#endif
        // compute extended bit
        ecc[3] ^= x & 0x01;
	}
    // add ecc[0-2] to extended parity
    for (idx=0; idx<2; idx++) {
    	uint8_t x = ecc[idx] ^ (ecc[idx]>>4);
		x ^= (x>>2);
        x ^= (x>>1);
        ecc[3] ^= x & 0x01;
    }
	return ((uint32_t)ecc[3] << 24) | ((uint32_t)ecc[2] << 16) | ((uint32_t)ecc[1] << 8) | ecc[0];
}

/**
 * Attempt to verify and correct a 512 byte buffer with the given ECC. If an error can be
 * corrected, it will be fixed in the passed buffer.
 * @param buffer the 512B buffer to verify
 * @param code the ECC code to verify with
 * @return true if no error or the error was corrected; false if the buffer has multi-bit errors.
 */
bool ecc_verify(uint8_t* buffer, uint32_t code) {
	uint32_t ecc_comp = ecc_generate(buffer) ^ code;
	if (ecc_comp == 0) return true; // No error
	// Count yon bits
	uint8_t i, bits;
	for (bits=i=0;i<24;i++) {
		if ((1<<i) & ecc_comp) bits++;
	}
	if (bits == 1) {
		// ECC code is corrupted
		return true;
	}
	if (bits == 12) {
		// Do correction
		if ((ecc_comp & (uint32_t)1<<24) == 0) return false; // Check extended DED bit
		uint8_t bit = ((ecc_comp & (uint32_t)1<<23)?4:0) |
				((ecc_comp & (uint32_t)1<<21)?2:0) |
				((ecc_comp & (uint32_t)1<<19)?1:0);
		uint16_t idx = ((ecc_comp & (uint32_t)1 << 17)?0x100:0) | (ecc_comp&0xff);
		buffer[idx] ^= 1 << bit;
		return true;
	}
	return false;
}

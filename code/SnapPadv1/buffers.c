/*
 * buffers.c
 *
 *  Created on: Mar 24, 2014
 *      Author: phooky
 */


#include "buffers.h"

static uint8_t buffer_A[PARA_SIZE+PARA_SPARE_SIZE];
static uint8_t buffer_B[PARA_SIZE+PARA_SPARE_SIZE];

static uint8_t* buffer_nand = buffer_A;
static uint8_t* buffer_rng = buffer_B;

uint8_t* buffers_get_nand() { return buffer_nand; }
uint8_t* buffers_get_rng() { return buffer_rng; }

void buffers_swap() {
	uint8_t* buffer_tmp = buffer_nand;
	buffer_nand = buffer_rng;
	buffer_rng = buffer_tmp;
}

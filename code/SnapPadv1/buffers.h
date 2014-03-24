/*
 * buffers.h
 *
 *  Created on: Mar 24, 2014
 *      Author: phooky
 */

#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <stdint.h>

uint8_t* buffers_get_nand();
uint8_t* buffers_get_rng();

void buffers_swap();

#endif /* BUFFERS_H_ */

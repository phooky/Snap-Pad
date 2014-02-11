/*
 * uarts.h
 *
 *  Created on: Jan 28, 2014
 *      Author: phooky
 */

#ifndef UARTS_H_
#define UARTS_H_

#include <stdint.h>
#include <stdbool.h>

enum {
	CS_INDETERMINATE = 0,
	CS_CONNECTED_MASTER = 1,
	CS_CONNECTED_SLAVE = 2,
	CS_NOT_CONNECTED = 3
};

typedef uint8_t ConnectionState;

/**
 * Init the UART for cross-chip communication.
 */
void uarts_init();

void uarts_send(uint8_t* buffer, uint16_t len);

/**
 * Based on the given master/slave assumption, figure out if the uart is connected and what state it's
 * in. (We can use this method to play the contention game if necessary- NYI)
 * @param is_master true if this side of the board is known to be in master mode
 * @return the connection state
 */
ConnectionState uarts_determine_state(bool is_master);

#endif /* UARTS_H_ */

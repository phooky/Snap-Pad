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
	CS_NOT_CONNECTED = 3,
	CS_COLLISION = 4
};

typedef uint8_t ConnectionState;

/**
 * Init the UART for cross-chip communication.
 */
void uarts_init();

void uarts_send(uint8_t* buffer, uint16_t len);

/**
 * Determine if the snap-pad is still connected to its twin, and if so, figure out if this snap-pad should operate
 * in master mode or slave mode. If the force_master flag is set, this half of the board will "cheat" and attempt
 * to become master by skipping the inital delay. This is useful in certain conditions, like starting a "factory
 * reset".
 * @param force_master true if this twin should cheat to become the master
 * @return the connection state
 */
ConnectionState uarts_determine_state(bool force_master);

#endif /* UARTS_H_ */

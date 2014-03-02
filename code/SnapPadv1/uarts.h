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
#include "config.h"

/**
 * Init the UART for cross-chip communication.
 */
void uart_init();

void uart_send(uint8_t* buffer, uint16_t len);

/*
 * Wait for the other side to confirm a factory reset
 */
void uart_factory_reset_confirm();


/**
 * Determine if the snap-pad is still connected to its twin, and if so, figure out if this snap-pad should operate
 * in master mode or slave mode. If the factory_reset flag is set, this half of the board will "cheat" and attempt
 * to become master by skipping the inital delay. This is useful in certain conditions, like starting a "factory
 * reset".
 * @param force_master true if this twin should cheat to become the master
 * @return the connection state
 */
ConnectionState uart_determine_state(bool force_master);

/** Process commands sent over the uart */
void uart_process();

#endif /* UARTS_H_ */

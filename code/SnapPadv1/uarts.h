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
 * Packet format:
 * TOK DAT*
 */

enum {
	// Tokens for master/slave contention
	UTOK_GAME_PING        = 0x10,
	UTOK_GAME_ACK         = 0x11,

	// Tokens for factory reset
	UTOK_RST_PROPOSE      = 0x12,
	UTOK_RST_CONFIRM      = 0x13,
	UTOK_RST_COMMIT       = 0x14,

	// Tokens for remote button press
	UTOK_BUTTON_QUERY     = 0x30,
	UTOK_BUTTON_RSP       = 0x31, // followed by state of button (1 byte)

	// Protocol for sending pages of data
	UTOK_BEGIN_DATA       = 0x23, // followed by 32-bit address, then 512 bytes
	UTOK_DATA_ACK         = 0x24, // data transfer successfully written
	UTOK_DATA_NAK         = 0x25, // data transfer failed

	UTOK_LAST
};


/**
 * Init the UART for cross-chip communication.
 */
void uart_init();


uint8_t uart_consume();
void uart_send_byte(uint8_t b);

/*
 * Wait for the other side to confirm a factory reset
 */
void uart_factory_reset_confirm();

void uart_send_buffer(uint8_t* buffer, uint16_t len);

/** Check if last bulk send is complete */
bool uart_send_complete();

/*
 * Ping remote end for a button push
 * @return true if remote twin has a button push
 */
bool uart_ping_button();

/**
 * Determine if the snap-pad is still connected to its twin, and if so, figure out if this snap-pad should operate
 * in master mode or slave mode. If the force_master flag is set, this half of the board will "cheat" and attempt
 * to become master by skipping the inital delay. This is useful in certain conditions, like starting a "factory
 * reset".
 * @param force_master true if this twin should cheat to become the master
 * @return the connection state
 */
ConnectionState uart_determine_state(bool force_master);

/** Process commands sent over the uart */
void uart_process();

#endif /* UARTS_H_ */

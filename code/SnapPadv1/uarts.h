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

/** The uart connection state indicates whether this Snap-Pad half is connected to
	another board via the UART, and if so, which role it plays. A Snap-Pad that is
	connected to an active USB connection should always be in the "Master" role. */
typedef uint8_t ConnectionState;

enum {
	CS_INDETERMINATE = 0,
	CS_SINGLE,
	CS_TWINNED_MASTER,
	CS_TWINNED_SLAVE,
	CS_TWINNED_COLLISION,

	CS_LAST
};

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

	UTOK_REQ_CHKSM        = 0x26, // followed by 16-bit block number
	UTOK_RSP_CHKSM        = 0x27, // followed by 16-bit checksum
	UTOK_RSP_CHKSM_BAD    = 0x28, // no followup; checksum is bad and block should be marked

	UTOK_MARK_BLOCK       = 0x29, // followed by 16-bit block number
	UTOK_MARK_ACK         = 0x2A, // block marked

	UTOK_LAST
};


/** Initialize the hardware UART. */
void uart_init();

/// Basic RX/TX

/** Retrieve one byte of data from the hardware uart. Will block perpetually
	until a byte of data is available. */
uint8_t uart_consume();

/** Attempt to retrieve one byte of data from the uart. If no data is available,
	wait for data until the given timeout, specified in milliseconds, passes.
	Data is returned in the one-character buffer. Returns true if data is valid,
	or false if the timeout has expired. */
bool uart_consume_timeout(uint8_t* buffer, uint16_t msec);

/** Wait until the UART is available for transmitting bytes, and then begin
	transmission of a single byte. */
void uart_send_byte(uint8_t b);

/** Wait until the UART is available for transmitting bytes, and then begin
	transmission of a single byte. */
void uart_send_buffer(uint8_t* buffer, uint16_t len);

/** Check if last transmission is complete. Return true if the uart has finished
	sending its last message and is ready for a new message. */
bool uart_send_complete();

/// Higher level UART operations

/** Propose a factory reset to the other board, and block until a confirmation
	message is recieved. (A factory reset requires that buttons be pressed on
	both halves of the board.) */
void uart_factory_reset_confirm();

/** Ping the other board to see if its button has been pushed. This is used
	to poll for a button push when a new board needs to be randomized. The
	user can push either button to initiate the randomization procedure.
	Returns true if the other board's button has been pushed. */
bool uart_ping_button();

/**
 * Determine if the snap-pad is still connected to its twin, and if so, figure out if this snap-pad should operate
 * in master mode or slave mode. If the force_master flag is set, this half of the board will "cheat" and attempt
 * to become master by skipping the inital delay. This is useful in certain conditions, like starting a "factory
 * reset".
 * @param force_master true if this twin should cheat to become the master
 * @param usb_present true if usb is present (favor usb side)
 * @return the connection state
 */
ConnectionState uart_determine_state(bool force_master, bool usb_present);

/** Process any pending commands that have been received over the uart. The board
	that is operating in the "Slave" role will call this repeatedly. */
void uart_process();

#endif /* UARTS_H_ */

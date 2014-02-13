/*
 * uarts.c
 *
 *  Created on: Jan 28, 2014
 *      Author: phooky
 */

#include "uarts.h"
#include "msp430.h"
#include "leds.h"
#include "hwrng.h"
#include <stdbool.h>

// Pins:
// P4.4 UCA1RXD
// P4.5 UCA1TXD

/**
 * Init the UART for cross-chip communication.
 */
void uart_init() {
	// Set pin directions
	P4SEL |= 1<<4 | 1<<5;
	// Put uart module in reset
	UCA1CTL1 |= UCSWRST;
	// Configure module
	UCA1CTL1 |= UCSSEL_2;
	UCA1BR0 = 9;
	UCA1BR1 = 0;
	UCA1MCTL |= UCBRS_1 + UCBRF_0;
	// Take uart module out of reset
	UCA1CTL1 &= ~UCSWRST;
	UCA1IE |= UCRXIE;
}

void uart_send(uint8_t* buffer, uint16_t len) {
	while (len > 0) {
		while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
		UCA1TXBUF = *(buffer++);
		len--;
	}
}


#define UART_RING_LEN 128
volatile uint8_t uart_ring_start = 0;
volatile uint8_t uart_ring_end = 0;
volatile uint8_t uart_ring_buf[UART_RING_LEN];

void uart_clear_buf() {
	UCA1IE &= ~UCRXIE;
	uart_ring_start = uart_ring_end = 0;
	UCA1IE |= UCRXIE;
}

bool uart_has_data() {
	return uart_ring_start != uart_ring_end;
}

uint8_t uart_consume() {
	uint8_t c = uart_ring_buf[uart_ring_start++];
	uart_ring_start = uart_ring_start % UART_RING_LEN;
	return c;
}


/**
 * Consume response buffer
 */
ConnectionState uart_state = CS_INDETERMINATE;

enum {
	// Protocol for master/slave contention and factory reset
	UTOK_GAME_PING        = 0x10,
	UTOK_GAME_ACK         = 0x11,
	UTOK_RST_PROPOSE      = 0x12,
	UTOK_RST_CONFIRM      = 0x13,
	UTOK_RST_COMMIT       = 0x14,

	// Protocol for sending pages of data
	UTOK_SELECT_BLOCK     = 0x20, // followed by 16-bit block #
	UTOK_SELECT_PAGE      = 0x21, // followed by 16-bit page #
	UTOK_BEGIN_DATA       = 0x23, // followed by 2112 bytes
	UTOK_DATA_ACK         = 0x24, // data transfer successfully written
	UTOK_DATA_NAK         = 0x25, // data transfer failed

	UTOK_LAST
};


/**
 * Play one round of the contention game. If neither side received the force_master flag,
 * they each wait a random amount of time before sending the ping packet. The first to acknowledge
 * the other side's ping is the slave. If the two packets conflict, the came ends in a conflict and
 * must be run again.
 * If after a timeout passes no token has been received, the device assumes that the pad has
 * been snapped and is disconnected from its twin.
 */
ConnectionState uart_play_round(bool force_master) {
	uint8_t remain = 100; // 100 ms total timeout
	uint16_t ms = 0;
	uart_clear_buf();
	__delay_cycles(8000); // allow uarts on both ends to come up
	if (!force_master) {
		hwrng_start();
		__delay_cycles(16000); // wait 2 ms
		while (!hwrng_done());
		ms = hwrng_bits()[0] & 0x3f; // wait up to 64 ms
		remain -= (ms + 2);
	}
	// scale both timing numbers by a factor of 10 to reduce the chance of jabber condition
	// when force_master is asserted on one or both boards
	remain *= 10;
	ms *= 10;
	while (ms--) {
		if (uart_has_data()) {
			if (uart_consume() == UTOK_GAME_PING) {
				UCA1TXBUF = UTOK_GAME_ACK;
				return CS_CONNECTED_SLAVE;
			} else {
				return CS_COLLISION;
			}
		}
		__delay_cycles(800);
	}
	UCA1TXBUF = UTOK_GAME_PING;
	while (remain--) {
		if (uart_has_data()) {
			if (uart_consume() == UTOK_GAME_ACK) {
				return CS_CONNECTED_MASTER;
			} else {
				return CS_COLLISION;
			}
		}
		__delay_cycles(800);
	}
	return CS_NOT_CONNECTED;
}

/**
 * Determine if the snap-pad is still connected to its twin, and if so, figure out if this snap-pad should operate
 * in master mode or slave mode. If the factory_reset flag is set, this half of the board will "cheat" and attempt
 * to become master by skipping the inital delay. This is useful in certain conditions, like starting a "factory
 * reset".
 * @param force_master true if this twin should cheat to become the master
 * @return the connection state
 */
ConnectionState uart_determine_state(bool force_master) {
	while (1) {
		ConnectionState cs = uart_play_round(force_master);
		if (cs != CS_COLLISION) {
			return uart_state = cs;
		}
		force_master = false; // If the cheat didn't work the first time, they're both cheating; play nice.
	}
}

/**
 * See if the uart is connected (which is to say, the other side is working and
 * still attached).
 * @return true if connected to other half of snap-pad
 */
bool uart_is_connected() {
	return (uart_state == CS_CONNECTED_MASTER) || (uart_state == CS_CONNECTED_SLAVE);
}

void uart_process() {

}
/**
 *
 */
// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
	uint8_t rx;
	switch(__even_in_range(UCA1IV,4))
	{
  	case 0:break;                             // Vector 0 - no interrupt
	case 2:                                   // Vector 2 - RXIFG
  		rx = UCA1RXBUF;
  		uart_ring_buf[uart_ring_end++] = rx;
  		uart_ring_end =  uart_ring_end % UART_RING_LEN;
  		break;
	case 4:break;                             // Vector 4 - TXIFG
	default: break;
	}
}


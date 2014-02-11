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
void uarts_init() {
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

void uarts_send(uint8_t* buffer, uint16_t len) {
	while (len > 0) {
		while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
		UCA1TXBUF = *(buffer++);
		len--;
	}
}


#define MSG_BUF_LEN 16
volatile uint8_t uarts_msg_len;
volatile uint8_t uarts_msg_buf[MSG_BUF_LEN];

/**
 * Prepare response buffer for another response.
 */
void uarts_clear_msg() {
	uarts_msg_len = 0;
}

ConnectionState uarts_state = CS_INDETERMINATE;

enum {
	UTOK_GAME_PING = 0x10,
	UTOK_GAME_ACK  = 0x11
};
/**
 * Based on the given master/slave assumption, figure out if the uart is connected and what state it's
 * in. (We can use this method to play the contention game if necessary- NYI)
 * @param is_master true if this side of the board is known to be in master mode
 * @return the connection state
 */
ConnectionState uarts_determine_state(bool is_master) {
	uint8_t i;
	uarts_clear_msg();
	__delay_cycles(16000); // wait 2 ms
	if (is_master) {
		UCA1TXBUF = UTOK_GAME_PING;
		for (i = 0; i < 100; i++) {
			if (uarts_msg_len > 0) {
				if (uarts_msg_buf[0] == UTOK_GAME_ACK) {
					return uarts_state = CS_CONNECTED_MASTER;
				} else {
					return uarts_state = CS_INDETERMINATE;
				}
			}
			__delay_cycles(800);
		}
	} else {
		for (i = 0; i < 100; i++) {
			if (uarts_msg_len > 0) {
				if (uarts_msg_buf[0] == UTOK_GAME_PING) {
					UCA1TXBUF = UTOK_GAME_ACK;
					return uarts_state = CS_CONNECTED_SLAVE;
				} else {
					return uarts_state = CS_INDETERMINATE;
				}
			}
			__delay_cycles(800);
		}
	}
	return uarts_state = CS_NOT_CONNECTED;
}


/**
 * See if the uart is connected (which is to say, the other side is working and
 * still attached).
 * @return true if connected to other half of snap-pad
 */
bool uarts_is_connected() {
	return (uarts_state == CS_CONNECTED_MASTER) || (uarts_state == CS_CONNECTED_SLAVE);
}

/**
 * NOTE: The UART contention game is on ice for now! Keeping the doc in case we need
 * it again.
 * ---
 * After boot, the device enters contention mode. Each half waits for 10ms, and then a randomly
 * determined delay from 0-20ms. If it receives the game token before that period times out, it
 * returns an acknowledgement token and goes into slave mode. Otherwise, it sends a game token.
 * If it  receives an acknowledgement token, it has won the game and goes into master mode. If
 * it has received a game token instead, a conflict has emerged and both sides restart the game.
 * If after a timeout passes no token has been received, the device assumes that the pad has
 * been snapped and is disconnected from its twin.
 */

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
  		uarts_msg_buf[uarts_msg_len++] = rx;
  		if (uarts_msg_len >= MSG_BUF_LEN) { uarts_msg_len--; }
  		break;
	case 4:break;                             // Vector 4 - TXIFG
	default: break;
	}
}


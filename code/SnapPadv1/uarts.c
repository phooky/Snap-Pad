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
#include "onetimepad.h"
#include "timer.h"
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

void uart_send_byte(uint8_t b) {
	while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
	UCA1TXBUF = b;
}


void uart_send(uint8_t* buffer, uint16_t len) {
	while (len > 0) {
		uart_send_byte(*(buffer++));
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
	// Tokens for master/slave contention
	UTOK_GAME_PING        = 0x10,
	UTOK_GAME_ACK         = 0x11,

	// Tokens for factory reset
	UTOK_RST_PROPOSE      = 0x12,
	UTOK_RST_CONFIRM      = 0x13,
	UTOK_RST_COMMIT       = 0x14,

	// Tokens for remote button press
	UTOK_BUTTON_QUERY     = 0x30,
	UTOK_BUTTON_RSP       = 0x31,

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
	timer_reset();
	uint16_t ms = 0;
	while (timer_msec() < 1) {} // let uarts settle
	if (!force_master) {
		hwrng_start();
		timer_reset(); while (timer_msec() < 2) {} // wait 2 ms
		while (!hwrng_done()) {} // wait for random data to become available
		ms = 2+(hwrng_bits()[0] & 0x3f); // additional delay of 2-66 ms
	}
	timer_reset();
	while (timer_msec() < ms) {
		if (uart_has_data()) {
			uint8_t b = uart_consume();
			if (b == UTOK_GAME_PING) {
				uart_send_byte(UTOK_GAME_ACK);
				return CS_TWINNED_SLAVE;
			} else if (b == UTOK_GAME_ACK) {
				return CS_TWINNED_COLLISION;
			}
		}
	}
	uart_send_byte(UTOK_GAME_PING);
	timer_reset();
	ms = 1000 - ms;
	while (timer_msec() < ms) {
		if (uart_has_data()) {
			uint8_t b = uart_consume();
			if (b == UTOK_GAME_ACK) {
				return CS_TWINNED_MASTER;
			} else if (b == UTOK_GAME_PING) {
				return CS_TWINNED_COLLISION;
			}
		}
	}
	return CS_SINGLE;
}

void uart_factory_reset_confirm() {
	uart_send_byte(UTOK_RST_PROPOSE);
	while (1) {
		if (uart_has_data() && (uart_consume() == UTOK_RST_CONFIRM)) return;
	}
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
		if (cs != CS_TWINNED_COLLISION) {
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
	return (uart_state == CS_TWINNED_MASTER) || (uart_state == CS_TWINNED_SLAVE);
}

void uart_process() {
	if (uart_has_data()) {
		uint8_t command = uart_consume();
		// Process factory reset commands:
		// UTOK_RST_PROPOSE, UTOK_RST_CONFIRM, UTOK_RST_COMMIT
		if (command == UTOK_RST_PROPOSE) {
			uint8_t i;
    		for (i = 0; i < LED_COUNT; i++) leds_set_led(i,LED_SLOW_1);
    		wait_for_confirm();
    		uart_send_byte(UTOK_RST_CONFIRM);
    		for (i = 0; i < LED_COUNT; i++) leds_set_led(i,(i%2 == 0)?LED_FAST_1:LED_FAST_0);
    		otp_factory_reset();
    		for (i = 0; i < LED_COUNT; i++) leds_set_led(i,LED_OFF);
    		while(1){} // Loop forever
		} else if (command == UTOK_BUTTON_QUERY) {
			uart_send_byte(UTOK_BUTTON_RSP);
			uart_send_byte(has_confirm()?0xff:0x00);
		}
	}
}

bool uart_ping_button() {
	uart_send_byte(UTOK_BUTTON_QUERY);
	while (!uart_has_data()) {}
	uint8_t rsp = uart_consume();
	if (rsp != UTOK_BUTTON_RSP) {
		usb_debug("BAD PINGRSP");
	}
	while (!uart_has_data()) {}
	rsp = uart_consume();
	return rsp != 0x00;
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


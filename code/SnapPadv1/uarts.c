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
#include "buffers.h"
#include "nand.h"
#include "print.h"

// Pins:
// P4.4 UCA1RXD
// P4.5 UCA1TXD

#define UART_RING_LEN 64
volatile uint8_t uart_rx_start = 0;
volatile uint8_t uart_rx_end = 0;
volatile uint8_t uart_rx_buf[UART_RING_LEN];

volatile uint8_t* uart_tx_buf = 0;
volatile uint16_t uart_tx_len = 0;

/**
 * Init the UART for cross-chip communication.
 */
void uart_init() {
	// Set pin directions
	P4SEL |= 1<<4 | 1<<5;
	// Put uart module in reset
	UCA1CTL1 |= UCSWRST;
	// Configure module
	UCA1CTL1 |= UCSSEL_1; // ACLK XT2 (12 Mhz)
	UCA1BR0 = 26;
	UCA1BR1 = 0;
	UCA1MCTL |= UCBRS_0 + UCBRF_0;
	//UCA1BR0 = 17;
	//UCA1BR1 = 0;
	//UCA1MCTL |= UCBRS_3 + UCBRF_0;
	// Take uart module out of reset
	UCA1CTL1 &= ~UCSWRST;
	UCA1IE |= UCRXIE;
	UCA1IE |= UCTXIE;
}

volatile bool ready_for_send = true;

uint8_t bytebuf;

void uart_send_byte(uint8_t b) {
	while (!uart_send_complete()) {}
	bytebuf = b;
	uart_send_buffer(&bytebuf,1);
}

void uart_send_buffer(uint8_t* buffer, uint16_t len) {
	while (!uart_send_complete()) {}
	ready_for_send = false;
	uart_tx_buf = buffer;
	uart_tx_len = len;
	if (uart_tx_len > 0) {
		uint8_t b = *(uart_tx_buf++);
		uart_tx_len--;
		//UCA1IE |= UCTXIE;
		UCA1TXBUF = b;
	}
}

/** Check if last bulk send is complete */
inline bool uart_send_complete() {
	return ready_for_send;
}

void uart_clear_buf() {
	UCA1IE &= ~UCRXIE;
	uart_rx_start = uart_rx_end = 0;
	UCA1IE |= UCRXIE;
}

bool uart_has_data() {
	return uart_rx_start != uart_rx_end;
}

uint8_t uart_consume() {
	while (uart_rx_start == uart_rx_end) {}
	uint8_t c = uart_rx_buf[uart_rx_start];
	uart_rx_start = (uart_rx_start +1) % UART_RING_LEN;
	return c;
}


uint8_t uart_consume_timeout(uint16_t timeout) {
	timer_reset();
	while (uart_rx_start == uart_rx_end) {
		if (timer_msec() >= timeout) { return 0xff; }
	}
	uint8_t c = uart_rx_buf[uart_rx_start];
	uart_rx_start = (uart_rx_start +1) % UART_RING_LEN;
	return c;
}

/**
 * Consume response buffer
 */
ConnectionState uart_state = CS_INDETERMINATE;


/**
 * Play one round of the contention game. If neither side received the force_master flag,
 * they each wait a random amount of time before sending the ping packet. The first to acknowledge
 * the other side's ping is the slave. If the two packets conflict, the came ends in a conflict and
 * must be run again.
 * If after a timeout passes no token has been received, the device assumes that the pad has
 * been snapped and is disconnected from its twin.
 * @param usb_present gives the usb-connected side precedence if force_master is not set.
 */
ConnectionState uart_play_round(bool force_master, bool usb_present) {
	timer_reset();
	uint16_t ms = 0;
	while (timer_msec() < 1) {} // let uarts settle
	if (!force_master) {
		hwrng_start();
		timer_reset(); while (timer_msec() < 2) {} // wait 2 ms
		while (!hwrng_done()) {} // wait for random data to become available
		if (!usb_present) {
			ms = 3+(hwrng_bits()[0] & 0x3f); // additional delay of 3-67 ms
		} else {
			ms = 0; // skip additional delay on usb-connected end
		}
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
 * @param usb_present true if usb is present (favor usb side)
 * @return the connection state
 */
ConnectionState uart_determine_state(bool force_master, bool usb_present) {
	while (1) {
		ConnectionState cs = uart_play_round(force_master,usb_present);
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
		} else if (command == UTOK_REQ_CHKSM) {
			uint16_t block;
			struct checksum_ret sum;
			block = uart_consume() << 8;
			block |= uart_consume();
			sum = nand_block_checksum(block);
			if (sum.ok) {
				uart_send_byte(UTOK_RSP_CHKSM);
				uart_send_byte(sum.checksum >> 8);
				uart_send_byte(sum.checksum & 0xff);
			} else {
				uart_send_byte(UTOK_RSP_CHKSM_BAD);
			}
		} else if (command == UTOK_MARK_BLOCK) {
			uint16_t block;
			block = uart_consume() << 8;
			block |= uart_consume();
			otp_mark_block(block,BU_BAD_BLOCK);
			uart_send_byte(UTOK_MARK_ACK);
		} else if (command == UTOK_BEGIN_DATA) {
			uint16_t i;
			uint8_t* buf;
			// get the address
			uint16_t block;
			uint8_t page;
			uint8_t para;

			block = uart_consume() << 8;
			block |= uart_consume();
			page = uart_consume();
			para = uart_consume();

			buf = buffers_get_nand();
			for (i = 0; i < PARA_SIZE; i++) {
				buf[i] = uart_consume();
			}

			if (page == 0 && para == 0) {
				leds_set_mode(LM_OFF);
				if (block > 256) { leds_set_led(3,LED_FAST_0); }
				if (block > 512+256) { leds_set_led(2,LED_FAST_0); }
				if (block > 1024+256) { leds_set_led(1,LED_FAST_0); }
				if (block > 1536+256) { leds_set_led(0,LED_FAST_0); }
				nand_block_erase(block);
				nand_wait_for_ready();
			}
			nand_save_para(block,page,para);
			nand_wait_for_ready();
			uart_send_byte(UTOK_DATA_ACK);
			if (block == 2047 && page == 63 && para == 3) {
				leds_set_mode(LM_DUAL_PROG_DONE);
				otp_set_flag(FLAG_DATA_FINISHED);
			} else if (block == 1 && page == 0 && para == 0) {
				otp_set_flag(FLAG_DATA_STARTED);
			}
		}
	}
}

bool uart_ping_button() {
	uart_clear_buf(); // clear the rx buffer
	uart_send_byte(UTOK_BUTTON_QUERY);
	while (!uart_has_data()) {}
	uint8_t rsp = uart_consume();
	if (rsp != UTOK_BUTTON_RSP) {
		print_usb_str("BAD PINGRSP ");
		print_usb_dec(rsp);
		print_usb_str("\n");
		return false;
	} else {
		while (!uart_has_data()) {}
		rsp = uart_consume();
		if (rsp != 0xff && rsp != 0x00) {
			print_usb_str("BAD PINGVAL ");
			print_usb_dec(rsp);
			print_usb_str("\n");
		}
		return rsp == 0xff;
	}
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
		// check for errors
		//if ((UCA1STAT & (UCOE|UCPE|UCBRK|UCRXERR)) != 0) {
	  	//	uart_rx_buf[uart_rx_end] = UCA1STAT;
	  	//	uart_rx_end = (uart_rx_end+1) % UART_RING_LEN;
		//}
  		rx = UCA1RXBUF;
  		uart_rx_buf[uart_rx_end] = rx;
  		uart_rx_end = (uart_rx_end+1) % UART_RING_LEN;
  		break;
	case 4:                                  // Vector 4 - TXIFG
		if (uart_tx_len == 0) {
			ready_for_send = true;
			//UCA1IE &= ~UCTXIE;
			// A word of explanation. uart_send_byte depends on this flag being set to indicate that
			// it's okay to send the next byte of data. It's set on reset, but cleared by this interrupt.
			//UCA1IFG |= UCTXIFG;
		} else {
			uart_tx_len--;
			UCA1TXBUF = *(uart_tx_buf++);
		}
		break;
	default: break;
	}
}


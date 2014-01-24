#include "nand.h"

#include <msp430f5508.h>
#include <stdbool.h>
#include "ecc.h"

/**
 * Pin assignments
 *
 * CLE - P4.2
 * ALE - P2.0
 * CE# - P4.3
 * WE# - P4.1
 * RE# - P4.6
 * WP# - P4.0
 *
 */

#define CLE_BIT BIT2
#define ALE_BIT BIT0
#define CEP_BIT BIT3
#define WEP_BIT BIT1
#define REP_BIT BIT6
#define WPP_BIT BIT0
#define RB_BIT BIT7

/** Output to NAND data bus.
 *  @param value 8-bit value to write to bus.
 */
inline void nand_io_write(uint8_t value) {
	P1OUT = value;
}

/** Read NAND data bus.
 *  @return value asserted on nand data bus.
 */
inline uint8_t nand_io_read() {
	return P1IN;
}
/** Set direction of NAND data bus.
 *  @param out true if output, false if input
 */
inline void nand_io_dir(bool out) {
		P1DIR = out?0xff:0x00;
}


inline void nand_set_ale(bool high) { P2OUT=high?(P2OUT | ALE_BIT):(P2OUT & ~ALE_BIT); }
inline void nand_set_cle(bool high) { P4OUT=high?(P4OUT | CLE_BIT):(P4OUT & ~CLE_BIT); }
inline void nand_set_ceP(bool high) { P4OUT=high?(P4OUT | CEP_BIT):(P4OUT & ~CEP_BIT); }
inline void nand_set_reP(bool high) { P4OUT=high?(P4OUT | REP_BIT):(P4OUT & ~REP_BIT); }
inline void nand_set_weP(bool high) { P4OUT=high?(P4OUT | WEP_BIT):(P4OUT & ~WEP_BIT); }
inline void nand_set_wpP(bool high) { P4OUT=high?(P4OUT | WPP_BIT):(P4OUT & ~WPP_BIT); }

/**
 * Initialize all the pins required to interact with the NAND
 * flash.
 */
void nand_init() {
	// Start with CE# high
	P4OUT |= CEP_BIT;
	P4DIR |= CEP_BIT;

	// Set other pins low
	P2DIR |= ALE_BIT;
	P2OUT &= ~ALE_BIT;
	P4DIR |= CLE_BIT | WEP_BIT | REP_BIT | WPP_BIT;
	P4OUT &= ~(CLE_BIT | WEP_BIT | REP_BIT | WPP_BIT);

	// Set R/B as input
	P4DIR &= ~RB_BIT;
	P4OUT |= RB_BIT; // pullup
	P4REN |= RB_BIT; // enable pullup/down

	// Default bus to input
	nand_io_dir(false);
	// Default WP off
	nand_set_wpP(true);
	// Default chip enable true
	nand_set_ceP(false);
}




inline bool nand_check_rb() {
	return (P4IN & RB_BIT) != 0;
}

inline void nand_wait_for_ready() {
	while (!nand_check_rb()) ;
}

void nand_send_command(uint8_t cmd) {
	nand_set_cle(true); nand_set_ale(false); nand_set_weP(false); nand_set_reP(true);
	nand_io_write(cmd);
	nand_io_dir(true);
	nand_set_weP(true);
}

void nand_send_address(uint32_t addr) {
	nand_set_cle(false); nand_set_ale(true); nand_set_weP(false); nand_set_reP(true);
	P1OUT = addr & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 8) & 0x0f;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 12) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 20) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 28) & 0x0f;
	nand_set_weP(true);
}

void nand_send_row_address(uint32_t addr) {
	nand_set_cle(false); nand_set_ale(true); nand_set_weP(false); nand_set_reP(true);
	P1OUT = (addr >> 12) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 20) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 28) & 0x0f;
	nand_set_weP(true);
}

void nand_send_byte_address(uint8_t baddr) {
	nand_set_cle(false); nand_set_ale(true); nand_set_weP(false); nand_set_reP(true);
	P1OUT = baddr;
	nand_set_weP(true);
}

void nand_recv_data(uint8_t* buffer, uint16_t count) {
	nand_set_cle(false); nand_set_ale(false); nand_set_weP(true); nand_set_reP(true);
	nand_io_dir(false);
	while (count--) {
		nand_set_reP(false);
		*(buffer++) = P1IN;
		nand_set_reP(true);
	}
}

void nand_send_data(const uint8_t* buffer, uint16_t count) {
	nand_set_cle(false); nand_set_ale(false); nand_set_weP(false); nand_set_reP(true);
	nand_io_write(0);
	nand_io_dir(true);
	while (count--) {
		nand_set_weP(false);
		nand_io_write(*(buffer++));
		nand_set_weP(true);
	}
}

void nand_send_zeros(uint16_t count) {
	nand_set_cle(false); nand_set_ale(false); nand_set_weP(false); nand_set_reP(true);
	nand_io_write(0);
	nand_io_dir(true);
	while (count--) {
		nand_set_weP(false);
		nand_io_write(0);
		nand_set_weP(true);
	}
}

IdInfo nand_read_id() {
	nand_send_command(0x90);
	nand_send_address(0x00);
	IdInfo info;
	nand_recv_data((uint8_t*)&info,sizeof(IdInfo));
	return info;
}

bool nand_check_ONFI() {
	nand_send_command(0x90);
	nand_send_address(0x20);
	uint8_t onfi[4];
	nand_recv_data((uint8_t*)onfi,4);
	return (onfi[0] == 'O') &&
			(onfi[1] == 'N') &&
			(onfi[2] == 'F') &&
			(onfi[3] == 'I');
}

uint8_t nand_read_status_reg() {
	nand_send_command(0x70);
	uint8_t buf;
	nand_recv_data(&buf,1);
	return buf;
}

void nand_block_erase(uint32_t address) {
	nand_send_command(0x60);
	nand_send_row_address(address);
	nand_send_command(0xd0);
	nand_wait_for_ready();
}

void nand_read_raw_page(uint32_t address, uint8_t* buffer, uint16_t count) {
	nand_send_command(0x00);
	nand_send_address(address);
	nand_send_command(0x30);
	nand_wait_for_ready();
	nand_recv_data(buffer,count);
}

bool nand_program_raw_page(const uint32_t address, const uint8_t* buffer, const uint16_t count) {
	nand_send_command(0x80);
	nand_send_address(address);
	nand_send_data(buffer,count);
	nand_send_command(0x10);
	return true;
}

void nand_read_parameter_page(uint8_t* buffer, uint16_t count) {
	nand_send_command(0xec);
	nand_send_address(0x00);
	nand_recv_data(buffer, count);
}

/**
 * Data+spare buffer. This is arranged as follows:
 * 0-511      paragraph 0
 * 512-1023   paragraph 1
 * 1024-1535  paragraph 2
 * 1536-2047  paragraph 3
 * 2048-2063  spare 0
 * 2064-2079  spare 1
 * 2080-2095  spare 2
 * 2096-2111  spare 3
 */
uint8_t page_buffer[2112];

/**
 * Load an entire page into the page buffer. Uses a Huffman code for SEC-DED on each 512B paragraph.
 * @param the address of the page start.
 * @return true if the read was successful; false if there was a multibit error.
 */
bool nand_load_page(uint32_t address) {
	nand_read_raw_page(address, page_buffer, 2112);
	uint8_t para;
	for (para = 0; para < 4; para++) {
		uint32_t ecc = *(uint32_t*)(page_buffer + SPARE_START + (16*para));
		if (!ecc_verify(page_buffer + (512*para),ecc)) return false;
	}
	return true;
}

/**
 * Write an entire page from the page buffer into NAND with SEC-DED error correction.
 * At present, blocks until entire page write is complete.
 * @param the address of the page start.
 * @return true if the write was successful; false if there was a write error.
 */
bool nand_save_page(uint32_t address) {
	uint8_t para;
	for (para = 0; para < 4; para++) {
		uint32_t ecc = ecc_generate(page_buffer + (512*para));
		*(uint32_t*)(page_buffer + SPARE_START + (16*para)) = ecc;
	}
	return nand_program_raw_page(address,page_buffer,2112);
}

/**
 * Retrieve a pointer to the 2112B page buffer. The area from 2048-2011 is the spare
 * data area; this data should rarely be directly manipulated by the client.
 */
uint8_t* nand_page_buffer() {
	return page_buffer;
}

/**
 * Zero a 512B paragraph and its associated spare area. This should be done immediately
 * after using a paragraph.
 * @param address the address of the page start
 * @param paragraph the paragraph within the page to erase (0-3).
 * @return true if the zero was successful
 */
bool nand_zero_paragraph(uint32_t address, uint8_t paragraph) {
	nand_send_command(0x80);
	nand_send_address(address+(paragraph*512));
	nand_send_zeros(512);
	nand_send_command(0x10);

	nand_wait_for_ready();

	nand_send_command(0x80);
	nand_send_address(address+ SPARE_START + (paragraph*16));
	nand_send_zeros(16);
	nand_send_command(0x10);

	nand_wait_for_ready();

	return true;
}

/**
 * Zero an entire page and its spare area.
 * @param adddress the address of the page start
 * @return true if the zero was successful
 */
bool nand_zero_page(uint32_t address) {
	nand_send_command(0x80);
	nand_send_address(address);
	nand_send_zeros(2112);
	nand_send_command(0x10);

	nand_wait_for_ready();

	return true;
}

#include "nand.h"

#include <msp430f5508.h>
#include <stdbool.h>
#include "ecc.h"
#include "buffers.h"

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

void nand_wait_for_ready() {
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

void nand_block_erase(uint16_t block) {
	nand_send_command(0x60);
	nand_send_row_address(nand_make_para_addr(block,0,0));
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
 * ***** WRONG ********
 * Data+spare buffer. This is arranged as follows:
 * 0-511      paragraph 0
 * 512-1023   paragraph 1
 * 1024-1535  paragraph 2
 * 1536-2047  paragraph 3
 * 2048-2063  spare 0
 * 2064-2079  spare 1
 * 2080-2095  spare 2
 * 2096-2111  spare 3
 * ***** TODO *** Update with current map (data/spare/data/spare/data/spare/data/spare)
 */


/**
 * Initialize the page buffer with unprogrammed (0xff) values.
 */
void nand_initialize_para_buffer() {
	uint16_t idx;
	uint8_t* para_buffer = buffers_get_nand();
	for (idx = 0; idx < PARA_SIZE; idx++) para_buffer[idx] = 0xff;
}

/**
 * Load an entire paragraph into the paragraph buffer. Uses a Huffman code for SEC-DED on each 512B paragraph.
 * @param block the block index
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the read was successful; false if there was a multibit error.
 */
bool nand_load_para(uint16_t block, uint8_t page, uint8_t paragraph) {
	uint32_t address = nand_make_para_addr(block,page,paragraph);
	uint8_t* para_buffer = buffers_get_nand();
	nand_read_raw_page(address, para_buffer, PARA_SIZE+PARA_SPARE_SIZE);
	uint32_t ecc = *(uint32_t*)(para_buffer + PARA_SIZE);
	if (!ecc_verify(para_buffer,ecc)) return false;
	return true;
}

/**
 * Compute the trivial checksum of a block.
 * @block the index of the block to generate a checksum for
 * @return the computed checksum
 */
struct checksum_ret nand_block_checksum(uint16_t block) {
	struct checksum_ret rv = {0,true};
	uint8_t page, para;
	uint8_t* para_buffer = buffers_get_nand();
	for (page = 0; page < PAGE_COUNT; page++) {
		for (para = 0; para < 4; para++) {
			if (nand_load_para(page/PAGE_COUNT,page%PAGE_COUNT,para)) {
				uint16_t idx;
				for (idx = 0; idx < 512; idx++) {
					rv.checksum += para_buffer[idx];
				}
			} else {
				rv.ok = false;
			}
		}
	}
	return rv;
}


/**
 * Write an entire page from the page buffer into NAND with SEC-DED error correction.
 * At present, blocks until entire page write is complete.
 * @param block the block index
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the write was successful; false if there was a write error.
 */
bool nand_save_para(uint16_t block, uint8_t page, uint8_t paragraph) {
	uint32_t address = nand_make_para_addr(block,page,paragraph);
	uint8_t* para_buffer = buffers_get_nand();
	uint32_t ecc = ecc_generate(para_buffer);
	*(uint32_t*)(para_buffer + PARA_SIZE) = ecc;
	return nand_program_raw_page(address,para_buffer,PARA_SIZE+PARA_SPARE_SIZE);
}

/**
 * Retrieve a pointer to the 528B page buffer. The area from 512B-528B is the spare
 * data area; this data should rarely be directly manipulated by the client.
 */
uint8_t* nand_para_buffer() {
	uint8_t* para_buffer = buffers_get_nand();
	return para_buffer;
}

/**
 * Zero a 512B paragraph and its associated spare area. This should be done immediately
 * after using a paragraph.
 * @param block the block index
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the zero was successful
 */
bool nand_zero_paragraph(uint16_t block, uint8_t page, uint8_t paragraph) {
	uint32_t address = nand_make_para_addr(block,page,paragraph);
	nand_send_command(0x80);
	nand_send_address(address);
	nand_send_zeros(PARA_SIZE+PARA_SPARE_SIZE);
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

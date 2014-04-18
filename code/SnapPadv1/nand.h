/*
 * nand.h
 *
 *  Created on: Dec 19, 2013
 *      Author: phooky
 */

#ifndef NAND_H_
#define NAND_H_
#include <stdint.h>
#include <stdbool.h>

#define PLANE_COUNT 2
#define BLOCK_COUNT 1024
#define PAGE_COUNT 64
#define SPARE_START 2048
#define PARA_SIZE 512
#define PARA_SPARE_SIZE 16

/**
 * Initialize pins and default state for NAND flash chip.
 */
void nand_init();

/**
 * Structure of the information returned by a read_id check.
 */
typedef struct {
	uint8_t manufacturer_code;
	uint8_t device_id;
	uint8_t details1;
	uint8_t details2;
	uint8_t ecc_info;
} IdInfo;

/**
 * Generate an address in NAND from a plane index, a block index, and
 * a column.
 */
inline uint32_t nand_make_addr(const uint32_t plane,const uint32_t block,const uint32_t page,const uint32_t column) {
	uint32_t addr = (column << 0) | (page << 12) | (plane << 18) | (block << 19);
	return addr;
}

/**
 * Generate an address in NAND from a block index (implies plane), a page number, and a paragraph number.
 */
inline uint32_t nand_make_para_addr(const uint16_t block,const uint8_t page,const uint8_t para) {
	uint32_t addr = ((PARA_SIZE+PARA_SPARE_SIZE)*para) | ((uint32_t)page << 12) | (((block & 0x400) == 0)?0:((uint32_t)1 << 18)) | ((uint32_t)(block & 0x3ff) << 19);
	return addr;
}

/**
 * Receive raw data from the NAND flash; useful for "continuations" of read_raw_page
 */
void nand_recv_data(uint8_t* buffer, uint16_t count);

/**
 * Read the ID of this NAND chip.
 * @return an IdInfo structure with the manufacturer code, device id, etc.
 */
IdInfo nand_read_id();

/**
 * Ensure that the chip supports the ONFI check.
 * @return true if the NAND is responding to the ONFI check.
 */
bool nand_check_ONFI();

/**
 * Get the value of the NAND's status register.
 * @return the 8-bit status flag register
 */
uint8_t nand_read_status_reg();

/**
 * Erase an entire 128KB block in NAND (resetting it to 0xff)
 * @param block the block to erase; plane is extracted from block #
 */
void nand_block_erase(uint16_t block);

/**
 * Read raw data from NAND. You can use nand_recv_data to continue to retrieve data
 * at successive addresses after the read completes.
 * @param address the full address to begin the read
 * @param buffer a sufficiently large buffer to read count bytes
 * @param count the number of bytes to retrieve from the NAND.
 */
void nand_read_raw_page(uint32_t address, uint8_t* buffer, uint16_t count);

/**
 * Compute the trivial checksum of a block.
 * @block the index of the block to generate a checksum for
 * @return the computed checksum
 */
uint16_t nand_block_checksum(uint16_t block);

/**
 * Write raw data to NAND.
 * @param address the full address to begin writing to
 * @param buffer the data to transfer to the NAND
 * @param count the number of bytes to write to the NAND.
 * @return true if write was successful; false if an error was detected.
 */
bool nand_program_raw_page(const uint32_t address, const uint8_t* buffer, const uint16_t count);

/**
 * Initialize the page buffer with unprogrammed (0xff) values.
 */
void nand_initialize_para_buffer();

/**
 * Load an entire paragraph into the paragraph buffer. Uses a Huffman code for SEC-DED on each 512B paragraph.
 * @param block the block number (>1024 indicates plane 1)
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the read was successful; false if there was a multibit error.
 */
bool nand_load_para(uint16_t block, uint8_t page, uint8_t paragraph);

/**
 * Write an entire page from the page buffer into NAND with SEC-DED error correction.
 * At present, blocks until entire page write is complete.
 * @param block the block number (>1024 indicates plane 1)
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the write was successful; false if there was a write error.
 */
bool nand_save_para(uint16_t block, uint8_t page, uint8_t paragraph);

/**
 * Retrieve a pointer to the 528B page buffer. The area from 512B-528B is the spare
 * data area; this data should rarely be directly manipulated by the client.
 */
uint8_t* nand_para_buffer();

/**
 * Zero a 512B paragraph and its associated spare area. This should be done immediately
 * after using a paragraph.
 * @param block the block number (>1024 indicates plane 1)
 * @param page the page number
 * @param paragraph the paragraph within the page to zero (0-3).
 * @return true if the zero was successful
 */
bool nand_zero_paragraph(uint16_t block, uint8_t page, uint8_t paragraph);

/**
 * Zero an entire page and its spare area.
 * @param adddress the address of the page start
 * @return true if the zero was successful
 */
bool nand_zero_page(uint32_t address);

/**
 * Read parameter page data into a buffer.
 */
void nand_read_parameter_page(uint8_t* buffer, uint16_t count);

/**
 * Wait for nand to finish any previous operations.
 */
void nand_wait_for_ready();

#endif /* NAND_H_ */

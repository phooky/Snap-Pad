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
 * Receive raw data from the NAND flash; only useful at chip boot or
 * after a command has been issued (and for reading past the end
 * of 'useful' buffers)
 */
void nand_recv_data(uint8_t* buffer, uint16_t count);


// for hax.
void nand_send_command(uint8_t cmd);
void nand_send_address(uint32_t addr);
void nand_send_byte_address(uint8_t baddr);

/**
 * Read the ID of this NAND chip.
 */
IdInfo nand_read_id();
/**
 * Ensure that the chip supports the ONFI check.
 */
bool nand_check_ONFI();

/**
 * Get the value of the NAND's status register.
 */
uint8_t nand_read_status_reg();
/**
 * Erase an entire 128KB block in NAND (resetting it to 0xff)
 */
void nand_block_erase(uint32_t address);
/**
 * Read data from NAND
 */
void nand_read_raw_page(uint32_t address, uint8_t* buffer, uint16_t count);
/**
 * Write data to NAND
 */
bool nand_program_raw_page(const uint32_t address, const uint8_t* buffer, const uint16_t count);

/**
 * Read parameter page data
 */
void nand_read_parameter_page(uint8_t* buffer, uint16_t count);

#endif /* NAND_H_ */

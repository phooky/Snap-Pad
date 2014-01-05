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
 * Receive raw data from the NAND flash; only useful at chip boot or
 * after a command has been issued (and for reading past the end
 * of 'useful' buffers)
 */
void nand_recv_data(uint8_t* buffer, uint16_t count);

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
 * Erase an entire 2KB block in NAND (resetting it to 0xff)
 */
void nand_block_erase(uint32_t address);
/**
 * Read data from NAND
 */
void nand_read_raw_page(uint32_t address, uint8_t* buffer, uint16_t count);
/**
 * Write data to NAND
 */
void nand_program_raw_page(uint32_t address, uint8_t* buffer, uint16_t count);

/**
 * Read parameter page data
 */
void nand_read_parameter_page(uint8_t* buffer, uint16_t count);

#endif /* NAND_H_ */

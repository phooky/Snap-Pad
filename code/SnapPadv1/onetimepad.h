#ifndef ONETIMEPAD_H
#define ONETIMEPAD_H

#include <stdint.h>
#include <stdbool.h>

#define BBL_MAX_ENTRIES 16

/**
 * Scan the raw NAND to detect bad blocks. Populate the passed bad block buffer.
 * @param bad_block_list buffer to populate with bad block ids. If the
 * list of scanned blocks is smaller than the given buffer length, the list will
 * be terminated with the value 0xFFFF
 * @param block_list_sz size of the bad block list buffer
 * @return the number of bad blocks found
 */
uint8_t otp_scan_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_sz);

/**
 * Retrieve the bad block list stored on the NAND.
 * @param bad_block_list buffer to populate with bad block ids
 * @param block_list_sz size of the bad block list buffer
 * @return the number of bad blocks loaded
 */
uint8_t otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_sz);

/**
 * Store the bad block list to the NAND.
 * @param bad_block_list list of bad block ids
 * @param block_list_count number of bad blocks
 * @return true if list successfully written
 */
bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count);

typedef struct {
	bool has_header;
	bool block_map_written;
	bool random_data_written;
	bool is_A;
	uint8_t major_version;
	uint8_t minor_version;
	uint16_t block_count;
} OTPConfig;

/**
 * Read header information.
 * @return a populated OTPConfig with the state of the OTP NAND
 */
OTPConfig otp_read_header();

/** Initialize the header block. If there's already one, erase block zero and recreate the
 * header block from scratch. Be careful! This method:
 * * Checks for and reads the bad block list
 *   * Generates a bad block list by scanning if none exists
 * * Erases block 0
 *   * Set-once flags and usage map are implicitly created by erasure (all 0xff)
 * * Creates and writes the header, version, and BBL
 *   * Marks header as written
 * * Creates the block mapping table
 *   * Marks non-present blocks in usage map as used
 *   * Marks block mapping table as created
 * @return true if successful
 */
bool otp_initialize_header();

#endif // ONETIMEPAD_H

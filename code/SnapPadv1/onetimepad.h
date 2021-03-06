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

/**
 * Erase entire nand chip (bad blocks excepted).
 */
void otp_factory_reset();

/** In-memory representation of OTP configuration. */
typedef struct {
	bool has_header;
	bool randomization_finished;
	bool randomization_started;
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
 * @return true if successful
 * @param is_A are we board A (consume blocks from start) or board B (consume blocks from end)
 */
bool otp_initialize_header(bool is_A);

/**
 * Run complete randomization process. Can take up to four hours to complete.
 */
bool otp_randomize_boards();

// The block usage page is a simple of map of the blocks of the chip; each block is represented by one byte. Block 0x00 is always marked as used.
enum {
	BU_UNUSED_BLOCK = 0xff,
	BU_USED_BLOCK = 0x00,
	BU_BAD_BLOCK = 0x70
};

/**
 * Mark a block in the block usage map
 */
void otp_mark_block(uint16_t block, uint8_t usage);


/**
 * Find the first/last unmarked block
 * @return the first/last available block, or 0xffff if none remain
 * @param backwards search backwards from the last block
 */
uint16_t otp_find_unmarked_block(bool backwards);

/**
 * Find the first/last usable page in the given block
 * @return true if a valid page is found
 * @param block the block to scan
 * @param page pointer to page index within block
 * @param backwards search backwards from the last block
 */
bool otp_find_unmarked_page(uint16_t block, uint16_t* page, bool backwards);

/**
 * Debug function: check usage status of a block
 * @param the number of the block
 * @return the usage status of the block (0xff if unused)
 */
uint8_t otp_get_block_status(uint16_t block);

/**
 * Provision a number of pages for use.
 * @param count The number of pages to provision, where 0 < count <= 4
 * @param is_A True if is this the A board half; false if B.
 */
void otp_provision(uint8_t count,bool is_A);

/**
 * Retrieve a particular page for decoding.
 * @param page page index
 */
void otp_retrieve(uint32_t page);

enum {
	FLAG_HEADER_WRITTEN,
	FLAG_DATA_STARTED,
	FLAG_DATA_FINISHED
};

/**
 * Set a flag indicating board state. Flags cannot be unmarked without a factory reset.
 * @param flag the flag to set (see the FLAG_* enumeration).
 */
void otp_set_flag(uint8_t flag);

#endif // ONETIMEPAD_H

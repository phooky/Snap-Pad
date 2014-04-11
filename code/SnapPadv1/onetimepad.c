#include "onetimepad.h"
#include "nand.h"
#include "config.h"
#include "leds.h"
#include "buffers.h"
#include "hwrng.h"
#include "uarts.h"

#define MAGIC_LEN 8
const uint8_t MAGIC[MAGIC_LEN] = { 'S','N','A','P','-','P','A','D' };

#define HEADER_PAGE 0
#define MAPPING_PAGE_START 2
#define USAGE_PAGE 4
#define FLAGS_PAGE 5

typedef struct {
	uint8_t magic[MAGIC_LEN];
	uint8_t major_version;
	uint8_t minor_version;
	uint16_t block_count;
} OTPHeader;

typedef struct {
	uint8_t header_written : 2;
	uint8_t block_map_written : 2;
	uint8_t random_data_written : 2;
	uint8_t reserved : 2;
} OTPFlags;

#define BBL_START 0x10

/** Check if a given block is marked as bad.
 * @param plane the plane the block is in (0-1)
 * @param block the block number (0-1023)
 * @return true if the block is marked bad; false otherwise
 */
bool check_bad_block(uint32_t plane, uint32_t block) {
	uint32_t addr;
	uint8_t check;
	addr = nand_make_addr(plane,block,0,SPARE_START);
	nand_read_raw_page(addr,&check,1);
	if (check != 0xff) return true;
	addr = nand_make_addr(plane,block,1,SPARE_START);
	nand_read_raw_page(addr,&check,1);
	if (check != 0xff) return true;
	addr = nand_make_addr(plane,block,PAGE_COUNT-1,SPARE_START);
	nand_read_raw_page(addr,&check,1);
	if (check != 0xff) return true;
	return false;
}

/** Mark a block as bad (for future scans). Presumes the block has been erased. */
void mark_bad_block(uint32_t plane, uint32_t block) {
	uint32_t addr;
	uint8_t check = 0;
	addr = nand_make_addr(plane,block,0,SPARE_START);
	nand_program_raw_page(addr,&check,1);
	nand_wait_for_ready();
	addr = nand_make_addr(plane,block,1,SPARE_START);
	nand_program_raw_page(addr,&check,1);
	nand_wait_for_ready();
	addr = nand_make_addr(plane,block,PAGE_COUNT-1,SPARE_START);
	nand_program_raw_page(addr,&check,1);
}

/**
 * Scan the raw NAND to detect bad blocks. Populate the passed bad block buffer.
 * @param bad_block_list buffer to populate with bad block ids. If the
 * list of scanned blocks is smaller than the given buffer length, the list will
 * be terminated with the value 0xFFFF
 * @param block_list_sz size of the bad block list buffer
 * @return the number of bad blocks found
 */
uint8_t otp_scan_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len) {
	uint32_t plane, block;
	uint8_t bbl_idx = 0;
	for (plane = 0; plane < PLANE_COUNT; plane++) {
		for (block = 0; block < BLOCK_COUNT; block++) {
			if (check_bad_block(plane,block)) {
				bad_block_list[bbl_idx++] = (plane << 10) | block;
				if (bbl_idx >= block_list_len) return bbl_idx;
			}
		}
	}
	bad_block_list[bbl_idx] = 0xFFFF;
	return bbl_idx;
}

/**
 * Erase entire nand chip (bad blocks excepted).
 */
void otp_factory_reset() {
	OTPConfig config = otp_read_header();
	uint16_t bbl[BBL_MAX_ENTRIES];
	uint8_t bbcount;
	uint16_t block;
	uint8_t bbidx;
	if (config.has_header) {
		bbcount = otp_fetch_bad_blocks(bbl, BBL_MAX_ENTRIES);
	} else {
		bbcount = otp_scan_bad_blocks(bbl, BBL_MAX_ENTRIES);
	}
	for (block = 0; block < BLOCK_COUNT*PLANE_COUNT; block++) {
		nand_block_erase(block);
	}
	for (bbidx = 0; bbidx < bbcount; bbidx++) {
		uint16_t plane = bbl[bbidx] >> 10;
		block = bbl[bbidx] & 0x3ff;
		mark_bad_block(plane,block);
	}
}

/**
 * Retrieve the bad block list stored on the NAND.
 * @param bad_block_list buffer to populate with bad block ids
 * @param block_list_sz size of the bad block list buffer
 * @return the number of bad blocks loaded
 */
uint8_t otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len) {
	uint32_t addr = nand_make_addr(0,0,0,BBL_START);
	nand_read_raw_page(addr,(uint8_t*)bad_block_list,block_list_len*2);
	uint8_t i;
	for (i = 0; i < block_list_len; i++) {
		if (bad_block_list[i] == 0xFFFF) { return i; }
	}
	return i;
}

/**
 * Store the bad block list to the NAND.
 * @param bad_block_list list of bad block ids
 * @param block_list_count number of bad blocks
 * @return true if list successfully written
 */
bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count) {
	uint32_t addr = nand_make_addr(0,0,0,BBL_START);
	nand_program_raw_page(addr,(uint8_t*)bad_block_list,bad_block_count*2);
	// if count is less than max, white 0xFF to end
	if (bad_block_count < BBL_MAX_ENTRIES) {
		nand_wait_for_ready();
		uint16_t entry = 0xff;
		nand_program_raw_page(addr + (bad_block_count*2), (uint8_t*)&entry, 2);
	}
	return true;
}

/** Header layout
 *  0x00: "SNAP-PAD" (8B)
 *  0x08: major version (1B)
 *  0x09: minor version (1B)
 *  0x10: block count (2B)
 *  0x0C: reserved (4B)
 *  0x10: bad block list (32B, 16 16-bit entries, terminated by 0xff)
 */

OTPConfig otp_read_header() {
	OTPConfig config;
	config.has_header = false;
	OTPHeader* header;
	uint8_t i;
	nand_load_para(0,0,0);
	header = (OTPHeader*)nand_para_buffer();
	for (i = 0; i < MAGIC_LEN; i++) {
		if (header->magic[i] != MAGIC[i]) return config; // no header found
	}
	config.has_header = true;
	config.major_version = header->major_version;
	config.minor_version = header->minor_version;
	config.block_count = header->block_count;

	OTPFlags flags;
	nand_read_raw_page(nand_make_addr(0,0,FLAGS_PAGE,0),(uint8_t*)&flags,sizeof(flags));
	config.block_map_written = flags.block_map_written != 0x03;
	config.random_data_written = flags.random_data_written != 0x03;
	return config;
}

inline uint16_t next_good(uint16_t last, uint16_t* bbl, uint8_t bbcount) {
	uint16_t next = last;
	uint8_t i;
	bool okay = false;
	while (!okay) {
		next++;
		okay = true;
		for (i = 0; i < bbcount; i++) {
			if (bbl[i] == next) okay = false;
		}
	}
	return next;
}

// debug proto
void usb_debug(char*);
void usb_debug_dec(int i);

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
bool otp_initialize_header() {
	OTPHeader* header;
	uint8_t i;
	uint16_t bbl[BBL_MAX_ENTRIES];
	uint8_t bbcount;
	// Check for existing header with BBL
	OTPConfig config = otp_read_header();\
	if (config.has_header) {
		bbcount = otp_fetch_bad_blocks(bbl, BBL_MAX_ENTRIES);
	} else {
		bbcount = otp_scan_bad_blocks(bbl, BBL_MAX_ENTRIES);
	}
	usb_debug("got bbl\n");
	// Erase block 0
	nand_block_erase(0);
	usb_debug("erased block 0\n");
	// Create and write header, version, bbl
	nand_initialize_para_buffer();
	header = (OTPHeader*)nand_para_buffer();
	for (i = 0; i < MAGIC_LEN; i++) {
		header->magic[i] = MAGIC[i];
	}
	header->major_version = MAJOR_VERSION;
	header->minor_version = MINOR_VERSION;
	header->block_count = 2048 - (1 + bbcount);
	usb_debug("prepared header\n");
	uint16_t* bbl_target = (uint16_t*)(nand_para_buffer() + BBL_START);
	for (i = 0; i < bbcount; i++) {
		*(bbl_target++) = bbl[i];
	}
	usb_debug("prepared bbl\n");
	// write header page
	bool write_succ = nand_save_para(0,0,0);
	usb_debug("wrote paragraph 0\n");
	nand_wait_for_ready();

	if (!write_succ) return false;
	// write header confirmation bits
	OTPFlags flags;
	uint32_t flagaddr = nand_make_addr(0,0,FLAGS_PAGE,0);
	nand_read_raw_page(flagaddr,(uint8_t*)&flags,sizeof(flags));
	nand_wait_for_ready();
	usb_debug("read flags\n");
	flags.header_written = 0x00;
	nand_program_raw_page(flagaddr,(uint8_t*)&flags,sizeof(flags));
	nand_wait_for_ready();
	usb_debug("wrote header-written flag\n");

	// create block mapping table
	nand_initialize_para_buffer();
	uint16_t idx = 0;
	uint16_t para = 0;
	uint16_t* map = (uint16_t*)nand_para_buffer();
	uint16_t next_free = next_good(0, bbl, bbcount);
	while (next_free < 2048) {
		map[idx] = next_free;
		idx++;
		if (idx == PARA_SIZE/2) {
			usb_debug("Writing paragraph ");
			usb_debug_dec(para);
			usb_debug("\n");
			nand_save_para(0,2+(para/4),para%4);
			nand_wait_for_ready();
			nand_initialize_para_buffer();
			para++;
			idx = 0;
		}
		next_free = next_good(next_free, bbl, bbcount);
	}
	if (idx > 0) {
		nand_save_para(0,2+(para/4),para%4);
		nand_wait_for_ready();
	}

	// mark block mapping table written
	flags.block_map_written = 0x00;
	nand_program_raw_page(flagaddr,(uint8_t*)&flags,sizeof(flags));
	nand_wait_for_ready();
	return true;
}

/**
 * Run complete randomization process. Can take up to four hours to complete.
 */
bool otp_randomize_boards() {
	/** TODO: sends. */
	uint16_t block;
	hwrng_bits_start(buffers_get_rng(),512);
	usb_debug("BEGIN RND\n");
	for (block = 1; block < 2048; block++) {
		uint8_t page;
		bool hwrngblock = false;
		bool uartblock = false;

		nand_block_erase(block);
		nand_wait_for_ready();

		leds_set_led(0,(block>0)?LED_FAST_0:LED_OFF);
		leds_set_led(1,(block>512)?LED_FAST_0:LED_OFF);
		leds_set_led(2,(block>1024)?LED_FAST_0:LED_OFF);
		leds_set_led(3,(block>1536)?LED_FAST_0:LED_OFF);
		for (page = 0; page < 64; page++) {
			uint8_t para;
			for (para = 0; para < 4; para++) {
				// Wait for RNG to finish filling buffer
				while (!hwrng_bits_done()) {
					hwrngblock = true;
				}
				// swap buffers
				buffers_swap();
				// restart rng
				hwrng_bits_start(buffers_get_rng(),512);
				// begin uart send
				uart_send_byte(UTOK_BEGIN_DATA);
				uart_send_byte(block >> 8);
				uart_send_byte(block & 0xff);
				uart_send_byte(page);
				uart_send_byte(para);
				uart_send_buffer(buffers_get_nand(),PARA_SIZE);
				// write to local nand
				nand_save_para(block,page,para);
				// wait for write completion
				nand_wait_for_ready();
				// wait for io confirmation
				while (!uart_send_complete()) { uartblock = true; }
				{
					uint8_t rsp;
					rsp = uart_consume();
					if (rsp == UTOK_DATA_ACK) {
						//usb_debug("OK RSP\n");
					} else {
						usb_debug("BAD RSP\n");
					}
				}
			}
		}
		if (hwrngblock) usb_debug("RNGBLK ");
		if (uartblock) usb_debug("UARTBLK ");

		usb_debug("BLOCK ");
		usb_debug_dec(block);
		usb_debug("\n");
	}
	leds_set_led(0,LED_SLOW_0);
	leds_set_led(1,LED_SLOW_0);
	leds_set_led(2,LED_SLOW_0);
	leds_set_led(3,LED_SLOW_0);
	return true;
}

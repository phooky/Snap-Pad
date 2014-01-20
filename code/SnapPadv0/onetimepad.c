#include "onetimepad.h"
#include "nand.h"
#include "config.h"

#define MAGIC_LEN 8
const uint8_t MAGIC[MAGIC_LEN] = { 'S','N','A','P','-','P','A','D' };

typedef struct {
	uint8_t magic[MAGIC_LEN];
	uint8_t version;
	uint8_t flags_a;
} OTPHeader;

#define BBL_START 0x10

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

void otp_scan_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len) {
	uint32_t plane, block;
	uint8_t bbl_idx = 0;
	for (plane = 0; plane < PLANE_COUNT; plane++) {
		for (block = 0; block < BLOCK_COUNT; block++) {
			if (check_bad_block(plane,block)) {
				bad_block_list[bbl_idx++] = (plane << 10) | block;
				if (bbl_idx >= block_list_len) return;
			}
		}
	}
	bad_block_list[bbl_idx] = 0xFFFF;
}

void otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len) {
	uint32_t addr = nand_make_addr(0,0,0,BBL_START);
	nand_read_raw_page(addr,(uint8_t*)bad_block_list,block_list_len*2);
}

bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count) {
	uint32_t addr = nand_make_addr(0,0,0,BBL_START);
	nand_program_raw_page(addr,(uint8_t*)bad_block_list,bad_block_count*2);
	// if count is less than max, white 0xFF to end
	if (bad_block_count < BBL_MAX_ENTRIES) {
		uint16_t entry = 0xff;
		nand_program_raw_page(addr + (bad_block_count*2), (uint8_t*)&entry, 2);
	}
	return true;
}

/** Header layout
 *  0x00: "SNAP-PAD" (8B)
 *  0x08: version (1B)
 *  0x09: flags_A (1B)
 *  0x0A: reserved (6B)
 *  0x10: bad block list (32B, 16 16-bit entries, terminated by 0xff)
 *
 *  Flags: each flag has two bits assigned to it.
 *  flags_A:
 *  -------------------------------------------------
 *  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 *  -------------------------------------------------
 *  |   BBL_W   |    BT_W   |  RDM_W    |  MASTER   |
 *  -------------------------------------------------
 *  BBL_W: 0x00 if bad block list has been written
 *  BT_W:  0x00 if block table has been written
 *  RDM_W: 0x00 if all random blocks have been initialized
 *  MASTER: 0x00 if this side of the snap-pad is the "master" for writing
 */

OTPConfig otp_read_header() {
	OTPConfig config;
	config.has_header = false;
	OTPHeader header;
	uint8_t i;
	nand_read_raw_page(0, (uint8_t*)&header, sizeof(OTPHeader));
	for (i = 0; i < MAGIC_LEN; i++) {
		if (header.magic[i] != MAGIC[i]) return config; // no header found
	}
	config.has_header = true;
	config.version = header.version;
	uint8_t flags_a = header.flags_a;
	config.bbl_written = (flags_a & 0xC0) != 0xC0;
	config.block_table_written = (flags_a & 0x30) != 0x30;
	config.bits_written = (flags_a & 0x0C) != 0x0C;
	config.is_master = (flags_a & 0x03) != 0x03;
	return config;
}

bool otp_initialize_header() {
	OTPHeader header;
	uint8_t i;
	for (i = 0; i < MAGIC_LEN; i++) {
		header.magic[i] = MAGIC[i];
	}
	header.version = VERSION;
	header.flags_a = 0xff;
	return nand_program_raw_page(0, (uint8_t*)&header, sizeof(OTPHeader));
}

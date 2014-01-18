#include "onetimepad.h"
#include "nand.h"

#define MAGIC_LEN 8
const uint8_t MAGIC[MAGIC_LEN] = { 'S','N','A','P','-','P','A','D' };

bool otp_has_magic() {
	uint8_t buffer[MAGIC_LEN];
	uint8_t i;
	nand_read_raw_page(0, buffer, MAGIC_LEN);
	for (i = 0; i < MAGIC_LEN; i++) {
		if (buffer[i] != MAGIC[i]) return false;
	}
	return true;
}

bool otp_write_magic() {
	return nand_program_raw_page(0, MAGIC, MAGIC_LEN);
}

void otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len) {

}

bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count) {
	return false;
}

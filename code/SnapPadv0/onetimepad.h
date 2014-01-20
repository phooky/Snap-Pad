#ifndef ONETIMEPAD_H
#define ONETIMEPAD_H

#include <stdint.h>
#include <stdbool.h>

#define BBL_MAX_ENTRIES 16
void otp_scan_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len);
void otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len);
bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count);

typedef struct {
	bool has_header;
	bool bbl_written;
	bool block_table_written;
	bool bits_written;
	bool is_master;
	uint8_t version;
} OTPConfig;

OTPConfig otp_read_header();
bool otp_initialize_header();



#endif // ONETIMEPAD_H

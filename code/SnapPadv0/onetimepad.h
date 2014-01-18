#ifndef ONETIMEPAD_H
#define ONETIMEPAD_H

#include <stdint.h>
#include <stdbool.h>

bool otp_has_magic();
bool otp_write_magic();

void otp_fetch_bad_blocks(uint16_t* bad_block_list, uint8_t block_list_len);
bool otp_write_bad_blocks(uint16_t* bad_block_list, uint8_t bad_block_count);

#endif // ONETIMEPAD_H

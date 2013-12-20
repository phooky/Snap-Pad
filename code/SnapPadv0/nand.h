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

void nand_init();

void nand_send_command(uint8_t cmd);
void nand_send_address(uint32_t addr);
void nand_recv_data(uint8_t* buffer, uint16_t count);

typedef struct {
	uint8_t manufacturer_code;
	uint8_t device_id;
	uint8_t details1;
	uint8_t details2;
	uint8_t ecc_info;
} IdInfo;

IdInfo nand_read_id();
bool nand_check_ONFI();

#endif /* NAND_H_ */

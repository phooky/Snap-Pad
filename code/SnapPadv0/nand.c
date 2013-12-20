#include "nand.h"

#include <msp430f5508.h>
#include <stdbool.h>

/**
 * Pin assignments
 *
 * CLE - P4.0
 * ALE - P2.0
 * CE# - P4.3
 * WE# - P4.2
 * RE# - P4.6
 * WP# - P4.1
 *
 */
#define CLE_BIT BIT0
#define ALE_BIT BIT0
#define CEP_BIT BIT3
#define WEP_BIT BIT2
#define REP_BIT BIT6
#define WPP_BIT BIT1

inline void nand_iodir(bool out,uint8_t value) {
	if (out) {
		P1OUT = value;
		P1DIR = 0xff;
	} else {
		P1DIR = 0x00;
	}
}

void nand_init() {
	// Start with CE# high
	P4OUT |= CEP_BIT;
	P4DIR |= CEP_BIT;

	// Set other pins low
	P2DIR |= ALE_BIT;
	P2OUT &= ~ALE_BIT;
	P4DIR |= CLE_BIT | WEP_BIT | REP_BIT | WPP_BIT;
	P4OUT &= ~(CLE_BIT | WEP_BIT | REP_BIT | WPP_BIT);

	nand_iodir(false,0);
}

#define P4BITS (CLE_BIT | WEP_BIT | REP_BIT | WPP_BIT | CEP_BIT)

inline void nand_set_mode(bool cle, bool ale, bool ceP, bool weP, bool reP, bool wpP) {
	uint8_t p2 = ale?ALE_BIT:0;
	uint8_t p4 = (cle?CLE_BIT:0) |
				(ceP?CEP_BIT:0) |
				(reP?REP_BIT:0) |
				(weP?WEP_BIT:0) |
				(ceP?CEP_BIT:0);
	P2OUT = (P2OUT & ~BIT0) | p2;
	P4OUT = (P4OUT & ~P4BITS) | p4;
}

inline void nand_set_weP(bool v) {
	if (v) {
		P4OUT |= WEP_BIT;
	} else {
		P4OUT &= ~WEP_BIT;
	}
}

inline void nand_set_reP(bool v) {
	if (v) {
		P4OUT |= REP_BIT;
	} else {
		P4OUT &= ~REP_BIT;
	}
}
void nand_send_command(uint8_t cmd) {
	nand_set_mode(true,false,false,false,true,false);
	nand_iodir(true,cmd);
	nand_set_weP(true);
}

void nand_send_address(uint32_t addr) {
	nand_set_mode(false,true,false,false,true,false);
	P1OUT = addr & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 8) & 0x0f;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 12) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 20) & 0xff;
	nand_set_weP(true);

	nand_set_weP(false);
	P1OUT = (addr >> 28) & 0x0f;
	nand_set_weP(true);
}

void nand_recv_data(uint8_t* buffer, uint16_t count) {
	nand_set_mode(false,false,false,true,true,false);
	nand_iodir(false,0);
	while (count--) {
		nand_set_reP(false);
		*(buffer++) = P1IN;
		nand_set_reP(true);
	}
}

IdInfo nand_read_id() {
	nand_send_command(0x90);
	nand_send_address(0x00);
	IdInfo info;
	nand_recv_data((uint8_t*)&info,sizeof(IdInfo));
	return info;
}


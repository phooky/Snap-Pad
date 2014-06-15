/*
 * config.h
 *
 *  Created on: Jan 20, 2014
 *      Author: phooky
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define MAJOR_VERSION 0x01
#define MINOR_VERSION 0x01

enum {
	CS_INDETERMINATE = 0,
	CS_SINGLE,
	CS_TWINNED_MASTER,
	CS_TWINNED_SLAVE,
	CS_TWINNED_COLLISION,

	CS_LAST
};

typedef uint8_t ConnectionState;
/**
 * Current connection state of snap-pad. Instantiated in main.c.
 */
extern ConnectionState connection_state;

//#define DEBUG

#endif /* CONFIG_H_ */

/*
 * config.h
 *
 *  Created on: Jan 20, 2014
 *      Author: phooky
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/** This configuration file builds the default release firmware by default.
	If you want to build the debug or factory test variants, define one
	of the following macros:
	DEBUG -- debugging build
	FACTORY_TEST -- factory test build
	*/

/** Define the major and minor versions of this firmware. The version is
	written as "M.mV", where M is the major version number, m is the
	minor version number, and V is the (optional) one character long
	build variant type. */
#define MAJOR_VERSION 1
#define MINOR_VERSION 1

/** The firmware variant is defined based on the values of the
	DEBUG and FACTORY_TEST flags. */
#if defined(DEBUG)
#define VARIANT "D"
#elif defined(FACTORY_TEST)
#define VARIANT "F"
#else
#define VARIANT ""
#endif

/** Define exactly one of options below to indicate which NAND
	chip the target board is using. */
#define NAND_CHIP_S34ML01G2			// 2Gb Samsung SLC flash

#endif /* CONFIG_H_ */

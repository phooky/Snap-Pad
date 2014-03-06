/*
 * timer.h
 *
 *  Created on: Mar 5, 2014
 *      Author: phooky
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void timer_init();

void timer_reset();

uint16_t timer_msec();


#endif /* TIMER_H_ */

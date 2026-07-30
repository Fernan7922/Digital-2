/*
 * Timer.h
 *
 * Created: 30/07/2026 15:13:24
 *  Author: ferg7
 */ 


#ifndef TIMER_H
#define TIMER_H

#include <avr/io.h>
#include <stdint.h>

void Timer_init(void);
uint32_t millis(void);

#endif
/*
 * Timer.h
 *
 * Created: 1/08/2026 02:00:16
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 

#ifndef TIMER_H
#define TIMER_H

#include <avr/io.h>
#include <stdint.h>

void Timer_init(void);
uint32_t millis(void);

#endif
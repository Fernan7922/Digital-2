/*
 * Timer.h
 *
 * Created: 30/07/2026 01:12:52
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef TIMER_H
#define TIMER_H

#include <avr/io.h>
#include <stdint.h>

void Timer_init(void);
uint32_t millis(void);
void wait_ms(uint32_t ms);

#endif
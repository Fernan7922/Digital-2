/*
 * Led.h
 *
 * Created: 30/07/2026 15:12:21
 *  Author: ferg7
 */ 


#ifndef LED_H
#define LED_H

#include <avr/io.h>

// LED indicador de bomba activa: D4 = PD4.
#define LED_DDR  DDRD
#define LED_PORT PORTD
#define LED_PIN  PD4

void LED_init(void);
void LED_on(void);
void LED_off(void);

#endif
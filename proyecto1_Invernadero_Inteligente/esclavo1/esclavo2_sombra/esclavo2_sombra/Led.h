/*
 * Led.h
 *
 * Created: 1/08/2026 01:58:26
 *  Author: ferg7
 */ 


#ifndef LED_H
#define LED_H

#include <avr/io.h>

// LED indicador de sombra activa: D6 = PD6.
#define LED_DDR  DDRD
#define LED_PORT PORTD
#define LED_PIN  PD6

void LED_init(void);
void LED_on(void);
void LED_off(void);

#endif
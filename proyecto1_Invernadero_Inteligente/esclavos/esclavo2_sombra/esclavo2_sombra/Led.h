/*
 * Led.h
 *
 * Created: 1/08/2026 01:58:26
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
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
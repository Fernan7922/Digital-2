/*
 * Led.h
 *
 * Created: 1/08/2026 03:00:16
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 

#ifndef LED_H
#define LED_H

#include <avr/io.h>

// LED indicador de ventilacion activa: D5 = PD5.
#define LED_DDR  DDRD
#define LED_PORT PORTD
#define LED_PIN  PD5

void LED_init(void);
void LED_on(void);
void LED_off(void);

#endif
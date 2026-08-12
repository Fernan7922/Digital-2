/*
 * Relay_fan.h
 *
 * Created: 1/08/2026 03:02:20
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef RELAY_FAN_H
#define RELAY_FAN_H

#include <avr/io.h>
//Este es usado para cuestiones de prueba del pulso hacia el relé-
// Relay del ventilador: Este modulo de relay es ACTIVO EN BAJO,
// (0 logico = relay cerrado = ventilador prendido), al reves que el de
// la bomba del Periferico 1. 
#define RELAY_FAN_DDR  DDRC
#define RELAY_FAN_PORT PORTC
#define RELAY_FAN_PIN  PC0

void Relay_fan_init(void);
void Relay_fan_on(void);
void Relay_fan_off(void);

#endif
/*
 * Relay_fan.h
 *
 * Created: 1/08/2026 03:02:20
 *  Author: ferg7
 */ 


#ifndef RELAY_FAN_H
#define RELAY_FAN_H

#include <avr/io.h>

// Relay del ventilador: A0 = PC0. Este modulo de relay es ACTIVO EN BAJO,
// (0 logico = relay cerrado = ventilador prendido), al reves que el de
// la bomba del Periferico 1. Por eso Relay_fan_init() arranca el pin en
// ALTO (ventilador apagado) y no en bajo.
#define RELAY_FAN_DDR  DDRC
#define RELAY_FAN_PORT PORTC
#define RELAY_FAN_PIN  PC0

void Relay_fan_init(void);
void Relay_fan_on(void);
void Relay_fan_off(void);

#endif
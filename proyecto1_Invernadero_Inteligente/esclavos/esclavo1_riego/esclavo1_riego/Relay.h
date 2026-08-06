/*
 * Relay.h
 *
 * Created: 30/07/2026 15:11:17
 *  Author: ferg7
 */ 

#ifndef RELAY_H
#define RELAY_H

#include <avr/io.h>

// Pin del relay que activa la bomba: D5 = PD5.
#define RELAY_DDR  DDRD
#define RELAY_PORT PORTD
#define RELAY_PIN  PD5

void Relay_init(void);
void Relay_on(void);
void Relay_off(void);

#endif
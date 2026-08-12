/*
 * Relay_fan.c
 *
 * Created: 1/08/2026 03:03:25
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "relay_fan.h"

void Relay_fan_init(void)
{
	RELAY_FAN_DDR |= (1 << RELAY_FAN_PIN);

	// Arranca en ALTO = ventilador APAGADO (porque el modulo es activo en bajo).
	RELAY_FAN_PORT |= (1 << RELAY_FAN_PIN);
}

void Relay_fan_on(void)
{
	// Se pone el pin en bajo para cerrar el relay y prender el ventilador.
	RELAY_FAN_PORT &= ~(1 << RELAY_FAN_PIN);
}

void Relay_fan_off(void)
{
	RELAY_FAN_PORT |= (1 << RELAY_FAN_PIN);
}
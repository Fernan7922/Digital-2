/*
 * Led.c
 *
 * Created: 1/08/2026 03:01:33
 *   Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "led.h"

void LED_init(void)
{
	LED_DDR |= (1 << LED_PIN);
	LED_PORT &= ~(1 << LED_PIN);
}

void LED_on(void)
{
	LED_PORT |= (1 << LED_PIN);
}

void LED_off(void)
{
	LED_PORT &= ~(1 << LED_PIN);
}
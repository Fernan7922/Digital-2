/*
 * Led.c
 *
 * Created: 1/08/2026 01:58:47
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 247347
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
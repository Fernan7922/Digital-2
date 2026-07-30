/*
 * Led.c
 *
 * Created: 30/07/2026 15:12:52
 *  Author: ferg7
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
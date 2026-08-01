/*
 * Timer.c
 *
 * Created: 1/08/2026 02:00:45
 *  Author: ferg7
 */ 
#include "timer.h"
#include <avr/interrupt.h>

static volatile uint32_t ms_counter = 0;

void Timer_init(void)
{
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01) | (1 << CS00); // prescaler 64
	OCR0A = 249;                        // 250 cuentas de 4us = 1ms
	TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
	ms_counter++;
}

uint32_t millis(void)
{
	uint32_t valor;
	cli();
	valor = ms_counter;
	sei();
	return valor;
}
/*
 * Timer.c
 *
 * Created: 30/07/2026 15:13:52
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "timer.h"
#include <avr/interrupt.h>

// Igual que en el Master: Timer0 en CTC generando 1 interrupcion cada 1ms.
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
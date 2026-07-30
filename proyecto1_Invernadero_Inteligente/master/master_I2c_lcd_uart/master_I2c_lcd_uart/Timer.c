/*
 * Timer.c
 *
 * Created: 30/07/2026 01:12:39
 *  Author: ferg7
 */ 

#include "timer.h"
#include <avr/interrupt.h>

// Contador de milisegundos. Es volatile porque una interrupcion lo modifica
// y el main lo lee; sin volatile el compilador podria "optimizar" esa lectura.
static volatile uint32_t ms_counter = 0;

void Timer_init(void)
{
	// Timer0 en modo CTC (Clear Timer on Compare Match): cuenta hasta OCR0A
	// y se reinicia solo, en vez de desbordarse en 255 como en modo normal.
	TCCR0A = (1 << WGM01);

	// Prescaler de 64: con F_CPU = 16MHz, cada cuenta del timer dura 4us.
	TCCR0B = (1 << CS01) | (1 << CS00);

	// 250 cuentas de 4us = 1000us = exactamente 1ms por interrupcion.
	OCR0A = 249;

	// Se habilita la interrupcion por coincidencia en el canal A (OCIE0A).
	TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
	ms_counter++;
}

uint32_t millis(void)
{
	uint32_t valor;

	// ms_counter es de 32 bits, y el AVR es de 8 bits: leerla toma varias
	// instrucciones. Si el ISR interrumpe a la mitad de esa lectura, se
	// podria leer un valor corrupto (mitad viejo, mitad nuevo). Por eso se
	// deshabilitan las interrupciones un instante mientras se copia el dato.
	cli();
	valor = ms_counter;
	sei();

	return valor;
}

void wait_ms(uint32_t ms)
{
	// Espera basada en el timer en vez de _delay_ms(). _delay_ms() cuenta
	// ciclos de CPU "a ciegas" (y se descalibra si cambia F_CPU o hay
	// interrupciones largas); aqui se compara contra millis(), que sigue
	// siendo exacto sin importar que otras interrupciones ocurran mientras
	// se espera.
	uint32_t inicio = millis();
	while ((millis() - inicio) < ms);
}
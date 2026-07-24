/*
 * adc.c
 *
 * Created: 23/07/2026 17:48:29
 *  Author: ferg7
 */ 
#include "adc.h"

void ADC_Init(void)
{
	// Referencia = AVcc (5V), resultado ajustado a la derecha (modo por defecto)
	ADMUX = (1 << REFS0);

	// Habilitamos el ADC y fijamos el prescaler en 128
	// 16MHz / 128 = 125kHz, que cae dentro del rango recomendado (50-200kHz) para tener precision de 10 bits
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_Read(uint8_t canal)
{
	// Limpiamos los bits de canal anteriores y seleccionamos el canal que nos interesa (A0-A7)
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);

	ADCSRA |= (1 << ADSC);              // arrancamos la conversion
	while (ADCSRA & (1 << ADSC));       // esperamos: el bit se pone en 0 solo cuando termina

	return ADC;                         // resultado de 10 bits: 0 - 1023
}
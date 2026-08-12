/*
 * Adc.c
 *
 * Created: 1/08/2026 01:59:52
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */
#include "adc.h"

void ADC_init(void)
{
	// AVCC (5V) como referencia.
	ADMUX = (1 << REFS0);

	// Prescaler 128 -> reloj del ADC en 125kHz, dentro del rango recomendado.
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read(uint8_t canal)
{
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
} 

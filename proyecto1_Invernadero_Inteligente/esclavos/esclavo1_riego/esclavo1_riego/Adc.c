/*
 * Adc.c
 *
 * Created: 30/07/2026 15:09:31
 *  Author: ferg7
 */ 

#include "adc.h"

void ADC_init(void)
{
	// REFS0 = 1: se usa AVCC (5V) como voltaje de referencia del ADC.
	ADMUX = (1 << REFS0);

	// ADEN habilita el modulo. El prescaler de 128 (ADPS2:0 = 111) deja
	// el reloj del ADC en 16MHz/128 = 125kHz, dentro del rango de 50-200kHz
	// que recomienda el datasheet para no perder precision.
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read(uint8_t canal)
{
	// Se limpian los bits de canal (MUX3:0) y se selecciona el nuevo,
	// sin tocar el bit REFS0 que ya se configuro en ADC_init().
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);

	// ADSC en 1 dispara una conversion; el hardware lo pone en 0 solo
	// cuando termina, asi que se espera a que eso pase.
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	// ADC es el registro de 16 bits que junta ADCL y ADCH automaticamente.
	return ADC;
}
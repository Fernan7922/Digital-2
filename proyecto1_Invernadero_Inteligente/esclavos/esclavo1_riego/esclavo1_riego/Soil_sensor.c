/*
 * Soil_sensor.c
 *
 * Created: 30/07/2026 15:10:51
 *  Author: ferg7
 */ 
#include "soil_sensor.h"
#include "adc.h"

// map() y constrain() son funciones propias del "core" de Arduino;
// en C puro no existen, asi que se reimplementan aqui, igual de simples.
static int32_t mapear(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int16_t limitar(int16_t x, int16_t minimo, int16_t maximo)
{
	if (x < minimo) return minimo;
	if (x > maximo) return maximo;
	return x;
}

uint16_t Soil_read_raw(void)
{
	return ADC_read(SOIL_ADC_CHANNEL);
}

int16_t Soil_read_percent(void)
{
	uint16_t crudo = Soil_read_raw();

	// Mismo mapeo que en tu ejemplo: VALOR_SECO -> 0%, VALOR_HUMEDO -> 100%.
	int16_t porcentaje = (int16_t)mapear(crudo, VALOR_SECO, VALOR_HUMEDO, 0, 100);

	return limitar(porcentaje, 0, 100);
}
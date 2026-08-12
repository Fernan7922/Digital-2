/*
 * Ldr_sensor.c
 *
 * Created: 1/08/2026 01:56:18
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 

#include "ldr_sensor.h"
#include "adc.h"

uint16_t LDR_read_raw(void)
{
	return ADC_read(LDR_ADC_CHANNEL);
}

uint8_t LDR_necesita_sombra(uint16_t valor_actual, uint8_t sombra_actual_activa)
{
	// Histeresis con 2 umbrales: si la sombra esta apagada, hace falta
	// subir hasta el umbral de activar (mucha luz) para prenderla; si ya
	// esta prendida, tiene que bajar hasta el umbral de desactivar (poca
	// luz) para que se retraiga. El rango entre ambos evita que el motor
	// este yendo y viniendo cuando la lectura anda justo en el limite.
	if (!sombra_actual_activa)
	{
		return (valor_actual > LDR_UMBRAL_ACTIVAR) ? 1 : 0;
	}
	else
	{
		return (valor_actual > LDR_UMBRAL_DESACTIVAR) ? 1 : 0;
	}
	
}

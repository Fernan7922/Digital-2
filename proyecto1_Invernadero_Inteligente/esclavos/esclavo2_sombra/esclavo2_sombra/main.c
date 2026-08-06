/*
 * esclavo2_sombra.c
 *
 * Created: 1/08/2026 01:53:00
 * Author : ferg7
 */ 
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"
#include "ldr_sensor.h"
#include "stepper.h"
#include "led.h"
#include "uart.h"
#include "timer.h"
#include "twi_slave.h"

#define INTERVALO_LECTURA_MS 1000UL

// Direccion I2C de este nodo, la que quedo asignada para el periferico
// de clima en el documento de pineado.
#define DIRECCION_I2C_PROPIA 0x09

int main(void)
{
	ADC_init();
	LED_init();
	Stepper_init();
	UART_init();
	Timer_init();

	TWI_slave_init(DIRECCION_I2C_PROPIA);

	sei();

	UART_print("Periferico 2 - Sombra listo\r\n");

	uint8_t sombra_activa = 0;
	uint32_t ultima_lectura = millis();

	while (1)
	{
		uint32_t ahora = millis();

		if ((ahora - ultima_lectura) >= INTERVALO_LECTURA_MS)
		{
			ultima_lectura = ahora;

			uint16_t valor = LDR_read_raw();
			uint8_t necesita_sombra = LDR_necesita_sombra(valor, sombra_activa);

			UART_print("LDR: ");
			UART_print_int((int16_t)valor);

			if (necesita_sombra && !sombra_activa)
			{
				Stepper_set_objetivo(STEPPER_PASOS_SOMBRA);
				LED_on();
				sombra_activa = 1;
				UART_print(" -> Sombra ACTIVADA\r\n");
			}
			else if (!necesita_sombra && sombra_activa)
			{
				Stepper_set_objetivo(0);
				LED_off();
				sombra_activa = 0;
				UART_print(" -> Sombra DESACTIVADA\r\n");
			}
			else
			{
				UART_print("\r\n");
			}

			// El valor crudo del LDR no cabe en 1 byte (el ADC llega
			// hasta 1023), asi que se manda partido en 2: la parte alta
			// y la parte baja. El master los vuelve a juntar de este
			// mismo lado del bus, no le toca adivinar nada.
			uint8_t paquete[3];
			paquete[0] = (uint8_t)(valor >> 8);
			paquete[1] = (uint8_t)(valor & 0xFF);
			paquete[2] = sombra_activa;
			TWI_slave_set_buffer(paquete, 3);
		}

		Stepper_update(ahora);
	}

	return 0;
}
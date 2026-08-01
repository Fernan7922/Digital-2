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

// Cada cuanto se lee el LDR y se decide si hay que mover el stepper.
#define INTERVALO_LECTURA_MS 1000UL

int main(void)
{
	ADC_init();
	LED_init();
	Stepper_init();
	UART_init();
	Timer_init();
	sei();

	UART_print("Periferico 2 - Sombra listo\r\n");

	uint8_t sombra_activa = 0;
	uint32_t ultima_lectura = millis();

	while (1)
	{
		uint32_t ahora = millis();

		// --- Lectura periodica del LDR (no bloqueante) ---
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
		}

		// El stepper se mueve un paso a la vez, cada vez que toca segun
		// STEPPER_INTERVALO_MS, sin bloquear el resto del programa mientras
		// tanto (por eso se llama en cada vuelta del loop, no solo cuando
		// se lee el LDR).
		Stepper_update(ahora);
	}

	return 0;
}

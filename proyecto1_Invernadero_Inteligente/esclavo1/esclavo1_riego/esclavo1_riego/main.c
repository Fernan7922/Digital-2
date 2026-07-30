/*
 * esclavo1_riego.c
 *
 * Created: 30/07/2026 15:08:13
 * Author : ferg7
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"
#include "soil_sensor.h"
#include "relay.h"
#include "led.h"
#include "uart.h"
#include "timer.h"

// Umbral de humedad: por debajo de esto, se considera que la tierra
// esta seca y se necesita regar.
#define UMBRAL_HUMEDAD 40

// Cuanto tiempo se deja la bomba encendida cada vez que riega.
#define DURACION_RIEGO_MS 5000UL

// Tiempo minimo de espera entre un riego y el siguiente, para no
// prender/apagar la bomba en cada lectura si el sensor anda al limite.
#define ENFRIAMIENTO_MS 10000UL

// Cada cuanto se lee el sensor y se manda el estado por UART.
#define INTERVALO_LECTURA_MS 1000UL

typedef enum
{
	ESTADO_IDLE,
	ESTADO_REGANDO
} estado_riego_t;

int main(void)
{
	ADC_init();
	Relay_init();
	LED_init();
	UART_init();
	Timer_init();
	sei();

	UART_print("Periferico 1 - Riego listo\r\n");

	estado_riego_t estado = ESTADO_IDLE;
	uint32_t ultima_lectura = millis();
	uint32_t inicio_riego = 0;
	uint32_t fin_enfriamiento = 0;

	while (1)
	{
		uint32_t ahora = millis();

		// --- Lectura periodica del sensor (no bloqueante) ---
		if ((ahora - ultima_lectura) >= INTERVALO_LECTURA_MS)
		{
			ultima_lectura = ahora;

			uint16_t crudo = Soil_read_raw();
			int16_t porcentaje = Soil_read_percent();

			UART_print("Crudo: ");
			UART_print_int((int16_t)crudo);
			UART_print(" | Humedad: ");
			UART_print_int(porcentaje);
			UART_print("%\r\n");

			// Solo se decide regar si estamos en IDLE, el suelo esta seco,
			// y ya paso el tiempo de enfriamiento del riego anterior.
			if (estado == ESTADO_IDLE &&
			porcentaje < UMBRAL_HUMEDAD &&
			ahora >= fin_enfriamiento)
			{
				Relay_on();
				LED_on();
				inicio_riego = ahora;
				estado = ESTADO_REGANDO;
				UART_print(">> Bomba ENCENDIDA\r\n");
			}
		}

		// --- Maquina de estados del riego (tambien no bloqueante) ---
		if (estado == ESTADO_REGANDO)
		{
			if ((ahora - inicio_riego) >= DURACION_RIEGO_MS)
			{
				Relay_off();
				LED_off();
				fin_enfriamiento = ahora + ENFRIAMIENTO_MS;
				estado = ESTADO_IDLE;
				UART_print(">> Bomba APAGADA\r\n");
			}
		}
	}

	return 0;
}
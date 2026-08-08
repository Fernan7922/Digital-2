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
#include "twi_slave.h"

#define INTERVALO_LECTURA_MS 1000UL
#define DIRECCION_I2C_PROPIA 0x08

int main(void)
{
	ADC_init();
	Relay_init();
	LED_init();
	UART_init();
	Timer_init();
	TWI_slave_init(DIRECCION_I2C_PROPIA);
	sei();

	UART_print("Periferico 1 - Riego listo (obedece al master)\r\n");

	uint8_t bomba_activa = 0;
	uint32_t ultima_lectura = millis();

	while (1)
	{
		uint32_t ahora = millis();

		// La decision de cuando regar ya no se toma aqui: el master es
		// quien la calcula (en automatico con sus propios umbrales, o en
		// manual con lo que llegue de Adafruit) y nos manda el resultado
		// por I2C. Este nodo solo obedece ese ultimo comando recibido.
		if (TWI_slave_hay_comando_nuevo())
		{
			bomba_activa = TWI_slave_leer_comando();

			if (bomba_activa)
			{
				Relay_on();
				LED_on();
				UART_print(">> Bomba ENCENDIDA (orden del master)\r\n");
			}
			else
			{
				Relay_off();
				LED_off();
				UART_print(">> Bomba APAGADA (orden del master)\r\n");
			}
		}

		// La lectura del sensor si se sigue haciendo aqui local (no tiene
		// sentido mandarle el ADC crudo al master para que nos diga que
		// leamos), solo que ahora unicamente reporta el dato, no decide.
		if ((ahora - ultima_lectura) >= INTERVALO_LECTURA_MS)
		{
			ultima_lectura = ahora;

			int16_t porcentaje = Soil_read_percent();

			UART_print("Humedad: ");
			UART_print_int(porcentaje);
			UART_print("%\r\n");

			uint8_t dato = (uint8_t)porcentaje;
			TWI_slave_set_buffer(&dato, 1);
		}
	}

	return 0;
}
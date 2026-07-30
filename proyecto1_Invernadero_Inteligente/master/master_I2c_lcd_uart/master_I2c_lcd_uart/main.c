/*
 * master_I2c_lcd_uart.c
 *
 * Created: 30/07/2026 01:09:07
 * Author : ferg7
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "twi.h"
#include "uart.h"
#include "timer.h"
#include "aht10.h"

int main(void)
{
	// Orden de inicializacion: primero los perifericos que no dependen de
	// interrupciones (UART, TWI), luego el timer, y hasta el final se
	// habilitan las interrupciones globales con sei().
	UART_init();
	TWI_init();
	Timer_init();
	sei();

	UART_print("Iniciando AHT10...\r\n");
	AHT10_init();

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000; // cada 2 segundos

	while (1)
	{
		// En vez de un delay bloqueante, se compara contra millis().
		// Asi el micro queda libre para (mas adelante) atender el bus I2C
		// como esclavo de otros nodos, sin quedar detenido en una espera.
		if ((millis() - ultima_lectura) >= intervalo_lectura)
		{
			float temperatura, humedad;

			if (AHT10_read(&temperatura, &humedad))
			{
				UART_print("Temp: ");
				UART_print_float(temperatura, 1);
				UART_print(" C  Hum: ");
				UART_print_float(humedad, 1);
				UART_print(" %\r\n");
			}
			else
			{
				UART_print("Sensor ocupado, se reintenta...\r\n");
			}

			ultima_lectura = millis();
		}
	}

	return 0;
}

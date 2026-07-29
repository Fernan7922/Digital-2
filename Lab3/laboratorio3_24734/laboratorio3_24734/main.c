/*
 * main.c  -  Nano ESCLAVO
 *
 * Lee 2 potenciometros por ADC, los convierte a voltaje (en
 * milivoltios) y manda ese voltaje por SPI (esclavo, con
 * interrupcion SPI_STC_vect) al Nano maestro, en tramas de 4 bytes:
 * [mv1_alto, mv1_bajo, mv2_alto, mv2_bajo]
 *
 * Tambien despliega el mismo voltaje por UART, para verificar en
 * una terminal aparte de la del maestro que la lectura esta bien.
 *
 * Conexiones:
 *   A0 -> potenciometro 1
 *   A1 -> potenciometro 2
 *   D13 (SCK), D12 (MISO), D11 (MOSI), D10 (SS) -> hacia el maestro
 *   GND comun con el maestro
 *   D0 (RX), D1 (TX) -> hacia la terminal (via USB del propio Nano)
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "adc.h"
#include "spi_slave.h"
#include "uart.h"

// Convierte una lectura ADC (0-1023) a milivoltios (0-5000mV),
// asumiendo referencia AVcc = 5V
uint16_t ConvertirAVoltaje(uint16_t valorADC)
{
	return ((uint32_t)valorADC * 5000) / 1023;
}

// Manda el voltaje (en mV) por UART con el formato "nombre: X.XXX V"
void MostrarVoltaje(const char *nombre, uint16_t mv)
{
	uint16_t entero = mv / 1000;
	uint16_t decimal = mv % 1000;

	char buffer[40];
	sprintf(buffer, "%s: %u.%03u V\r\n", nombre, entero, decimal);
	UART_TransmitString(buffer);
}

int main(void)
{
	ADC_Init();
	SPI_Slave_Init();
	UART_Init(9600);
	sei();   // habilita interrupciones globales (necesario para SPI_STC_vect)

	while (1)
	{
		uint16_t pot1 = ADC_Read(0);   // lectura cruda del potenciometro en A0 (0-1023)
		uint16_t pot2 = ADC_Read(1);   // lectura cruda del potenciometro en A1 (0-1023)

		uint16_t mv1 = ConvertirAVoltaje(pot1);  // 0-5000 mV
		uint16_t mv2 = ConvertirAVoltaje(pot2);  // 0-5000 mV

		// Mandamos el VOLTAJE (en milivoltios) por SPI, ya convertido.
		// mv1 y mv2 caben perfectamente en 16 bits (max 5000 < 65535),
		// asi que se sigue partiendo en byte alto / byte bajo igual
		// que antes, solo que ahora es voltaje y no el valor crudo
		SPI_Slave_ActualizarTrama(mv1 >> 8, mv1 & 0xFF, mv2 >> 8, mv2 & 0xFF);

		// Mostramos el mismo voltaje por UART para verificar
		MostrarVoltaje("Pot1", mv1);
		MostrarVoltaje("Pot2", mv2);

		_delay_ms(500);   // pequena pausa para no saturar la terminal
	}
}
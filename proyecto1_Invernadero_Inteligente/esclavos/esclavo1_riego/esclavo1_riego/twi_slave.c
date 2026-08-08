/*
 * twi_slave.c
 *
 * Created: 5/08/2026 22:55:04
 *  Author: ferg7
 */ 
#include "twi_slave.h"
#include <avr/interrupt.h>

// Buffer de salida: lo que este nodo entrega cuando el master lo LEE.
static volatile uint8_t buffer[TWI_SLAVE_MAX_BUFFER];
static volatile uint8_t buffer_longitud = 0;
static volatile uint8_t indice_tx = 0;

// Lo que el master nos ESCRIBE (el comando del actuador) se guarda aca.
// comando_nuevo queda en 1 hasta que el loop principal lo revisa, para
// no perder un comando si llega justo entre 2 vueltas del loop.
static volatile uint8_t comando_recibido = 0;
static volatile uint8_t comando_nuevo = 0;

void TWI_slave_init(uint8_t direccion_7bits)
{
	TWAR = (direccion_7bits << 1);
	TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
}

void TWI_slave_set_buffer(const uint8_t *datos, uint8_t longitud)
{
	if (longitud > TWI_SLAVE_MAX_BUFFER)
	{
		longitud = TWI_SLAVE_MAX_BUFFER;
	}

	cli();
	for (uint8_t i = 0; i < longitud; i++)
	{
		buffer[i] = datos[i];
	}
	buffer_longitud = longitud;
	indice_tx = 0;
	sei();
}

uint8_t TWI_slave_hay_comando_nuevo(void)
{
	return comando_nuevo;
}

uint8_t TWI_slave_leer_comando(void)
{
	cli();
	comando_nuevo = 0;
	uint8_t valor = comando_recibido;
	sei();
	return valor;
}

ISR(TWI_vect)
{
	uint8_t estado = TWSR & 0xF8;

	switch (estado)
	{
		// --- El master nos esta pidiendo datos (nosotros transmitimos) ---
		case 0xA8: // nos direccionaron para leer
		case 0xB8: // ya mandamos un byte y el master contesto ACK
		if (indice_tx < buffer_longitud)
		{
			TWDR = buffer[indice_tx++];
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		}
		else
		{
			TWDR = 0xFF;
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
		}
		break;

		case 0xC0: // el master contesto NACK, ya no quiere mas
		case 0xC8:
		indice_tx = 0;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		// --- El master nos esta mandando un comando (nosotros recibimos) ---
		case 0x60: // nos direccionaron para escribir
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		case 0x80: // llego el byte de comando
		comando_recibido = TWDR;
		comando_nuevo = 1;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		default:
		// Cualquier otro estado se libera el bus para no dejarlo trabado.
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;
	}
}
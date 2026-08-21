/*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
*/
#include "twi_slave.h"
#include <avr/interrupt.h>
static volatile uint8_t buffer[TWI_SLAVE_MAX_BUFFER];
static volatile uint8_t buffer_longitud = 0;
static volatile uint8_t indice = 0;

// Ultimo comando que el Master nos escribio, y bandera de si ya se
// leyo o todavia esta pendiente.
static volatile uint8_t comando_recibido = 0;
static volatile uint8_t hay_comando_nuevo = 0;

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
	indice = 0;
	sei();
}

uint8_t TWI_slave_hay_comando(void)
{
	return hay_comando_nuevo;
}

uint8_t TWI_slave_leer_comando(void)
{
	hay_comando_nuevo = 0;
	return comando_recibido;
}

ISR(TWI_vect)
{
	uint8_t estado = TWSR & 0xF8;
	switch (estado)
	{
		case 0xA8:
		case 0xB8:
		if (indice < buffer_longitud)
		{
			TWDR = buffer[indice++];
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		}
		else
		{
			TWDR = 0xFF;
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
		}
		break;
		case 0xC0:
		case 0xC8:
		indice = 0;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		// Nos estan escribiendo un byte (el Master nos manda un
		// comando, en vez de venir a leer datos).
		case 0x80:
		case 0x88:
		comando_recibido = TWDR;
		hay_comando_nuevo = 1;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		default:
		indice = 0;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;
	}
}
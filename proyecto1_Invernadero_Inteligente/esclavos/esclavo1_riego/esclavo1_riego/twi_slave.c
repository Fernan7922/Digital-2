/*
 * twi_slave.c
 *
 * Created: 5/08/2026 22:55:04
 *  Author: ferg7
 */ 
#include "twi_slave.h"
#include <avr/interrupt.h>

// Este buffer es lo que el ISR del TWI va entregando byte por byte cuando
// el master hace una lectura. Se actualiza desde el loop principal (fuera
// de interrupcion) cada vez que hay una lectura nueva del sensor.
static volatile uint8_t buffer[TWI_SLAVE_MAX_BUFFER];
static volatile uint8_t buffer_longitud = 0;
static volatile uint8_t indice = 0;

void TWI_slave_init(uint8_t direccion_7bits)
{
	// TWAR guarda la direccion propia de 7 bits, corrida un bit a la
	// izquierda porque el bit 0 de TWAR es para reconocimiento de
	// "general call" (que aqui no se usa).
	TWAR = (direccion_7bits << 1);

	// TWEA: se contesta con ACK cuando el master nos direcciona.
	// TWIE: cada evento del bus (direccion recibida, byte pedido, etc.)
	// dispara la interrupcion TWI_vect en vez de tener que estar
	// revisando el estado del bus a cada rato desde el loop principal.
	TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
}

void TWI_slave_set_buffer(const uint8_t *datos, uint8_t longitud)
{
	if (longitud > TWI_SLAVE_MAX_BUFFER)
	{
		longitud = TWI_SLAVE_MAX_BUFFER;
	}

	// Se protege la copia porque el ISR puede estar leyendo el buffer
	// justo en el momento en que el loop principal lo quiere actualizar.
	cli();
	for (uint8_t i = 0; i < longitud; i++)
	{
		buffer[i] = datos[i];
	}
	buffer_longitud = longitud;
	indice = 0;
	sei();
}

ISR(TWI_vect)
{
	uint8_t estado = TWSR & 0xF8;

	switch (estado)
	{
		case 0xA8: // nos acaban de direccionar y quieren leer (SLA+R + ACK)
		case 0xB8: // ya se mando un byte y el master contesto ACK (quiere mas)
		if (indice < buffer_longitud)
		{
			TWDR = buffer[indice++];
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		}
		else
		{
			// Ya no queda nada que mandar en este buffer; se rellena
			// con 0xFF para que el master lo pueda reconocer como
			// "no hay mas datos" si intenta seguir leyendo.
			TWDR = 0xFF;
			TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
		}
		break;

		case 0xC0: // se mando un byte y el master contesto NACK (ya no quiere mas)
		case 0xC8: // se mando el ultimo byte del buffer y aun asi llego ACK
		// Se reinicia el indice para que la proxima vez que el master
		// pregunte, la lectura empiece otra vez desde el primer byte.
		indice = 0;
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;

		default:
		// Cualquier otro estado (por ejemplo si el master intentara
		// escribirnos en vez de leernos, cosa que este proyecto no usa)
		// se libera el bus para no dejarlo trabado.
		TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
		break;
	}
}
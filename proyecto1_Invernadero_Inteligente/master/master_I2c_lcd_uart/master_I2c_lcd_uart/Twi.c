/*
 * Twi.c
 *
 * Created: 30/07/2026 01:11:12
 *  Author: ferg7
 */ 

#include "twi.h"
#define F_CPU 16000000UL
void TWI_init(void)
{
	// Se usa un prescaler de 1 (TWPS1:0 = 00 en TWSR), y con eso se calcula
	// TWBR para que la formula del bit rate de el valor de SCL que queremos.
	TWSR &= ~((1 << TWPS1) | (1 << TWPS0));
	TWBR = (uint8_t)(((F_CPU / TWI_SCL_FREQ) - 16) / 2);
}

void TWI_start(void)
{
	// TWSTA en 1 pide el bus y genera la condicion de arranque (START).
	// TWINT se escribe en 1 para "soltar" al hardware y que ejecute la accion.
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	// El hardware vuelve a poner TWINT en 1 cuando termina; ahi se sigue.
	while (!(TWCR & (1 << TWINT)));
}

void TWI_stop(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

	// La condicion de parada (STOP) no levanta TWINT; en vez de eso,
	// se espera a que el propio bit TWSTO se limpie solo.
	while (TWCR & (1 << TWSTO));
}

uint8_t TWI_write(uint8_t data)
{
	// El dato (direccion+R/W, o un byte de datos) se carga en TWDR
	// y se limpia TWINT para que se transmita por el bus.
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));

	return TWI_get_status();
}

uint8_t TWI_read_ack(void)
{
	// TWEA en 1: al terminar de recibir el byte, se manda ACK
	// (le indica al esclavo que se quiere seguir leyendo mas bytes).
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));

	return TWDR;
}

uint8_t TWI_read_nack(void)
{
	// Sin TWEA se manda NACK: le dice al esclavo que este es el ultimo byte.
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));

	return TWDR;
}

uint8_t TWI_get_status(void)
{
	// Los 2 bits bajos de TWSR son el prescaler, no el codigo de estado real,
	// por eso se enmascaran antes de comparar contra las tablas del datasheet.
	return (TWSR & 0xF8);
}

uint8_t TWI_read_from_slave(uint8_t direccion_7bits, uint8_t *buffer, uint8_t longitud)
{
	TWI_start();

	// 0x40 es el codigo de estado para "SLA+R enviada, ACK recibido"
	// (osea que el esclavo si contesto). Si no coincide, se cierra el
	// bus con STOP y se avisa que esta lectura no sirvio.
	uint8_t estado = TWI_write((direccion_7bits << 1) | 1);
	if (estado != 0x40)
	{
		TWI_stop();
		return 0;
	}

	for (uint8_t i = 0; i < longitud; i++)
	{
		if (i == (longitud - 1))
		{
			// Al ultimo byte se le manda NACK, como en cualquier lectura I2C,
			// para que el esclavo sepa que ya no hace falta que siga mandando.
			buffer[i] = TWI_read_nack();
		}
		else
		{
			buffer[i] = TWI_read_ack();
		}
	}

	TWI_stop();
	return 1;
}
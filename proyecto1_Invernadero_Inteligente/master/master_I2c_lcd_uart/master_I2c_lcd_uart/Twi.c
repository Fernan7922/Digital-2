*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
#include "twi.h"
#define F_CPU 16000000UL
void TWI_init(void)
{
	TWSR &= ~((1 << TWPS1) | (1 << TWPS0));
	TWBR = (uint8_t)(((F_CPU / TWI_SCL_FREQ) - 16) / 2);
}

void TWI_start(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void TWI_stop(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	while (TWCR & (1 << TWSTO));
}

uint8_t TWI_write(uint8_t data)
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return TWI_get_status();
}

uint8_t TWI_read_ack(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t TWI_read_nack(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t TWI_get_status(void)
{
	return (TWSR & 0xF8);
}

uint8_t TWI_read_from_slave(uint8_t direccion_7bits, uint8_t *buffer, uint8_t longitud)
{
	TWI_start();

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
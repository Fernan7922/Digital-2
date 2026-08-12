*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
#ifndef TWI_H
#define TWI_H

#include <avr/io.h>
#include <stdint.h>

#define TWI_SCL_FREQ 100000UL

void TWI_init(void);
void TWI_start(void);
void TWI_stop(void);
uint8_t TWI_write(uint8_t data);
uint8_t TWI_read_ack(void);
uint8_t TWI_read_nack(void);
uint8_t TWI_get_status(void);
uint8_t TWI_read_from_slave(uint8_t direccion_7bits, uint8_t *buffer, uint8_t longitud);

#endif
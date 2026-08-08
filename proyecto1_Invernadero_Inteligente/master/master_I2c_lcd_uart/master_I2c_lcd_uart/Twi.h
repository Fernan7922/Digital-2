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

// Lee "longitud" bytes desde el esclavo de la direccion indicada.
// Devuelve 1 si el esclavo contesto, 0 si no hubo respuesta.
uint8_t TWI_read_from_slave(uint8_t direccion_7bits, uint8_t *buffer, uint8_t longitud);

// Manda 1 byte de comando al esclavo indicado (por ejemplo, encender o
// apagar su actuador). Devuelve 1 si el esclavo contesto el ACK de
// direccion, 0 si no hubo respuesta.
uint8_t TWI_write_to_slave(uint8_t direccion_7bits, uint8_t dato);

#endif
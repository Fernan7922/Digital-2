#ifndef TWI_H
#define TWI_H

#include <avr/io.h>
#include <stdint.h>

// Frecuencia de SCL deseada para el bus I2C (modo estandar).
#define TWI_SCL_FREQ 100000UL

void TWI_init(void);
void TWI_start(void);
void TWI_stop(void);
uint8_t TWI_write(uint8_t data);
uint8_t TWI_read_ack(void);
uint8_t TWI_read_nack(void);
uint8_t TWI_get_status(void);

// Lee "longitud" bytes desde el esclavo de la direccion indicada y los
// deja en buffer. Se arma con las mismas funciones de arriba (start,
// direccion+R, leer con ACK menos el ultimo byte, stop), asi que sigue
// siendo generico: no sabe que hay un periferico de riego o de clima al
// otro lado, solo sabe hablar el protocolo I2C.
// Devuelve 1 si el esclavo contesto, 0 si no hubo respuesta (por ejemplo
// si ese Nano todavia no ha arrancado o esta desconectado del bus).
uint8_t TWI_read_from_slave(uint8_t direccion_7bits, uint8_t *buffer, uint8_t longitud);

#endif
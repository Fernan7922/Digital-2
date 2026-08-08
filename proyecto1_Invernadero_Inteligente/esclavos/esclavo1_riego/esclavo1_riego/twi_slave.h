/*
 * twi_slave.h
 *
 * Created: 5/08/2026 22:54:29
 *  Author: ferg7
 */ 


#ifndef TWI_SLAVE_H
#define TWI_SLAVE_H

#include <avr/io.h>
#include <stdint.h>

#define TWI_SLAVE_MAX_BUFFER 4

void TWI_slave_init(uint8_t direccion_7bits);

// Se llama cada vez que el nodo termina de leer su sensor, para dejar
// listos los bytes que se entregaran la proxima vez que el master pida
// datos (modo transmisor del esclavo).
void TWI_slave_set_buffer(const uint8_t *datos, uint8_t longitud);

// El master ahora tambien nos puede escribir un byte de comando (modo
// receptor del esclavo). Estas 2 funciones son la forma de que el loop
// principal se entere de eso, sin tener que tocar nada dentro del ISR.
uint8_t TWI_slave_hay_comando_nuevo(void);
uint8_t TWI_slave_leer_comando(void);

#endif
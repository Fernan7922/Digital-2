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

// Tamano maximo de datos que este nodo puede entregar en una sola lectura
// del master. 4 bytes alcanza de sobra para lo que manda cada periferico
// de este proyecto (riego usa 2, clima usa 3).
#define TWI_SLAVE_MAX_BUFFER 4

// direccion_7bits es la direccion I2C del propio nodo (0x08, 0x09, etc.),
// la misma que quedo asignada en el documento de pineado.
void TWI_slave_init(uint8_t direccion_7bits);

// Se llama cada vez que el nodo termina de leer sus sensores, para dejar
// listos los bytes que se entregaran la proxima vez que el master pregunte.
// No manda nada por si solo: el maestro es quien decide cuando leer.
void TWI_slave_set_buffer(const uint8_t *datos, uint8_t longitud);

#endif
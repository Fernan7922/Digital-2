/*  Proyecto Invernadero - BE3029 Electronica Digital 2
 Juan Daniel Sandoval 24209 y Fernando Guzman 24734
*/
#ifndef TWI_SLAVE_H
#define TWI_SLAVE_H
#include <avr/io.h>
#include <stdint.h>
#define TWI_SLAVE_MAX_BUFFER 4

void TWI_slave_init(uint8_t direccion_7bits);
void TWI_slave_set_buffer(const uint8_t *datos, uint8_t longitud);

// Comandos que llegan del Master (cuando el es el que escribe hacia
// este esclavo, en vez de venir a leer). TWI_slave_hay_comando() avisa
// si llego uno nuevo desde la ultima vez que se pregunto, y
// TWI_slave_leer_comando() lo entrega y baja la bandera.
uint8_t TWI_slave_hay_comando(void);
uint8_t TWI_slave_leer_comando(void);

#endif
/*
 * spi_slave.h
 *
 * Libreria SPI para el Nano ESCLAVO, usando el modulo de
 * hardware del ATmega328p, con interrupcion SPI_STC_vect,
 * segun el material del curso IE3054 - Modulo MSSP SPI.
 *
 * Author: ferg7
 */
#ifndef SPI_SLAVE_H_
#define SPI_SLAVE_H_

#include <avr/io.h>

void SPI_Slave_Init(void);

// Carga una nueva trama de 4 bytes que se ira enviando al maestro,
// un byte por cada transferencia que el pida. Solo se aplica si el
// esclavo no esta a la mitad de mandar la trama anterior.
void SPI_Slave_ActualizarTrama(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

#endif /* SPI_SLAVE_H_ */
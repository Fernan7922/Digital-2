/*
 * Twi.h
 *
 * Created: 30/07/2026 01:11:31
 *  Author: ferg7
 */ 


#ifndef TWI_H
#define TWI_H

#include <avr/io.h>

// Frecuencia de SCL deseada para el bus I2C (modo estandar).
#define TWI_SCL_FREQ 100000UL

void TWI_init(void);
void TWI_start(void);
void TWI_stop(void);
uint8_t TWI_write(uint8_t data);
uint8_t TWI_read_ack(void);
uint8_t TWI_read_nack(void);
uint8_t TWI_get_status(void);

#endif
/*
 * Adc.h
 *
 * Created: 30/07/2026 15:08:50
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 

#ifndef ADC_H
#define ADC_H

#include <avr/io.h>
#include <stdint.h>

void ADC_init(void);
uint16_t ADC_read(uint8_t canal);

#endif
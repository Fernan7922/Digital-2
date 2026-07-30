/*
 * Adc.h
 *
 * Created: 30/07/2026 15:08:50
 *  Author: ferg7
 */ 

#ifndef ADC_H
#define ADC_H

#include <avr/io.h>
#include <stdint.h>

void ADC_init(void);
uint16_t ADC_read(uint8_t canal);

#endif
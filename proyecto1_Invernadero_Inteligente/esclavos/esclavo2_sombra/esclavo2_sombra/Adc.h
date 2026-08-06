/*
 * Adc.h
 *
 * Created: 1/08/2026 01:59:19
 *  Author: ferg7
 */ 

#ifndef ADC_H
#define ADC_H

#include <avr/io.h>
#include <stdint.h>

void ADC_init(void);
uint16_t ADC_read(uint8_t canal);

#endif
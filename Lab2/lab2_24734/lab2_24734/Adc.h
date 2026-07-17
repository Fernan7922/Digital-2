/*
 * Adc.h
 *
 * Created: 16/07/2026 17:54:27
 *  Author: ferg7
 */ 


#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>

void ADC_Init(void);
uint16_t ADC_Read(uint8_t canal);

#endif
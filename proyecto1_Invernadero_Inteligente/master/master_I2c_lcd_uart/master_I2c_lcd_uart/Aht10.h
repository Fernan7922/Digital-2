/*
 * Aht10.h
 *
 * Created: 30/07/2026 01:13:31
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef AHT10_H
#define AHT10_H

#include <stdint.h>

#define AHT10_ADDRESS 0x38

uint8_t AHT10_init(void);
uint8_t AHT10_read(float *temperatura, float *humedad);

#endif
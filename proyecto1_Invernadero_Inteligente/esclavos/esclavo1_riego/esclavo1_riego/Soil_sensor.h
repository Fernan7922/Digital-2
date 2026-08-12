/*
 * Soil_sensor.h
 *
 * Created: 30/07/2026 15:10:06
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdint.h>

#define SOIL_ADC_CHANNEL 0

// Valores de calibracion 
#define VALOR_SECO   590
#define VALOR_HUMEDO 300

uint16_t Soil_read_raw(void);
int16_t Soil_read_percent(void);

#endif
/*
 * Soil_sensor.h
 *
 * Created: 30/07/2026 15:10:06
 *  Author: ferg7
 */ 


#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdint.h>

#define SOIL_ADC_CHANNEL 0

// Valores de calibracion (ajustar con tu sensor real, igual que en tu
// ejemplo original de Arduino).
#define VALOR_SECO   590
#define VALOR_HUMEDO 300

uint16_t Soil_read_raw(void);
int16_t Soil_read_percent(void);

#endif
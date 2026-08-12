/*
 * Ldr_sensor.h
 *
 * Created: 1/08/2026 01:54:16
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 

#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include <stdint.h>

#define LDR_ADC_CHANNEL 0

// =====================================================================
// CALIBRACION DEL LDR
// =====================================================================
// Cableado: LDR entre 5V y el nodo A0, resistencia de 10k entre A0 y GND.
// Con esa configuracion, mientras mas luz le pega al LDR, su resistencia
// baja, cae menos voltaje sobre el LDR, y el nodo A0 queda mas cerca de
// 5V -> el numero que lee el ADC SUBE con mas luz y BAJA con poca luz.
//
// Comportamiento que queremos: mucha luz activa la sombra, poca luz la
// retrae. Por eso aqui se compara con ">" y no con "<".
//
// Para calibrar:se debe tapar el LDR con la mano y anotar el numero que sale por
// UART (eso es "poca luz"), luego destapar bajo luz normal de cuarto y
// anota ese otro numero ("mucha luz"). Poner el umbral de activar un poco
// abajo del numero de "mucha luz", y el de desactivar un poco abajo de
// ese, dejando un rango entre los 2 (no pegados) para que no este
// prendiendo y apagando el motor en cada lectura.
#define LDR_UMBRAL_ACTIVAR    50   // por ARRIBA de este valor se activa sombra
#define LDR_UMBRAL_DESACTIVAR 20   // por DEBAJO de este valor se retrae sombra
// =====================================================================

uint16_t LDR_read_raw(void);
uint8_t LDR_necesita_sombra(uint16_t valor_actual, uint8_t sombra_actual_activa);

#endif
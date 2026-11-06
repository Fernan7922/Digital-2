#ifndef TFT_PARQUEO_H
#define TFT_PARQUEO_H
#include <stdint.h>
uint8_t TFT_Parqueo_Init(void);
void TFT_Parqueo_Actualizar(uint8_t bits, uint8_t valid);
uint8_t TFT_Parqueo_Error(void);
#endif

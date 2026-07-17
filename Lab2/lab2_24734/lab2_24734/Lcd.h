/*
 * Lcd.h
 *
 * Created: 16/07/2026 17:52:55
 *  Author: ferg7
 */ 


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>

// ==================== Definicion de pines ====================
// Si en algun momento cambia el cableado, solo hay que tocar esto
// y el resto de la libreria sigue funcionando igual.

#define LCD_DATA_PORT   PORTD   // D0-D7 de la LCD van conectados a PORTD completo (Arduino D0-D7)
#define LCD_DATA_DDR    DDRD

#define LCD_CTRL_PORT   PORTB   // RS y E van en PORTB (Arduino D8 y D9)
#define LCD_CTRL_DDR    DDRB

#define LCD_RS   PB0   // Arduino D8
#define LCD_E    PB1   // Arduino D9

// Nota: R/W esta amarrado directo a GND en el cableado, por eso no aparece
// ningun pin ni funcion de lectura aqui. Solo escribimos hacia la LCD.

void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_SetCursor(uint8_t fila, uint8_t columna);
void LCD_Print(const char *str);
void LCD_Clear(void);

#endif




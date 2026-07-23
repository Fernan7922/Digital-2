/*
 * Lcd.h
 *
 * Created: 16/07/2026 17:52:55
 *  Author: ferg7
 */ 


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>

// Definicion de pines 
// Si en algun momento cambia el cableado, solo hay que tocar esto
// y el resto de la libreria sigue funcionando igual.
//
// IMPORTANTE: se tenía un conflicto en pines :ya no usamos PORTD completo. Antes D0-D7 de la LCD
// estaban en PORTD0-PORTD7, pero esos mismos pines (D0/D1 del Arduino)
// los necesita el UART (RXD/TXD), asi que movimos las lineas de datos
// para dejar esos dos pines libres. Cada linea de datos ahora se
// define por separado, para poder repartirlas en distintos puertos.

#define LCD_RS_PORT   PORTB
#define LCD_RS_DDR    DDRB
#define LCD_RS_PIN    PB0        // Arduino D8

#define LCD_E_PORT    PORTB
#define LCD_E_DDR     DDRB
#define LCD_E_PIN     PB1        // Arduino D9

#define LCD_D0_PORT   PORTB
#define LCD_D0_DDR    DDRB
#define LCD_D0_PIN    PB2        // Arduino D10 (antes era D0, se movio por el UART)

#define LCD_D1_PORT   PORTB
#define LCD_D1_DDR    DDRB
#define LCD_D1_PIN    PB3        // Arduino D11 (antes era D1, se movio por el UART)

#define LCD_D2_PORT   PORTD
#define LCD_D2_DDR    DDRD
#define LCD_D2_PIN    PD2        // Arduino D2 (sin cambio)

#define LCD_D3_PORT   PORTD
#define LCD_D3_DDR    DDRD
#define LCD_D3_PIN    PD3        // Arduino D3 (sin cambio)

#define LCD_D4_PORT   PORTD
#define LCD_D4_DDR    DDRD
#define LCD_D4_PIN    PD4        // Arduino D4 (sin cambio)

#define LCD_D5_PORT   PORTD
#define LCD_D5_DDR    DDRD
#define LCD_D5_PIN    PD5        // Arduino D5 (sin cambio)

#define LCD_D6_PORT   PORTD
#define LCD_D6_DDR    DDRD
#define LCD_D6_PIN    PD6        // Arduino D6 (sin cambio)

#define LCD_D7_PORT   PORTD
#define LCD_D7_DDR    DDRD
#define LCD_D7_PIN    PD7        // Arduino D7 (sin cambio)

// Nota: R/W esta amarrado directo a GND en el cableado, por eso no aparece
// ningun pin ni funcion de lectura aqui. Solo escribimos hacia la LCD.

void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_SetCursor(uint8_t fila, uint8_t columna);
void LCD_Print(const char *str);
void LCD_Clear(void);

#endif




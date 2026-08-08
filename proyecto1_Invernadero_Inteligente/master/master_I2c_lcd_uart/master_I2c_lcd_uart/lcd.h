

/*
 * lcd.h
 *
 * Driver bare-metal para LCD 16x2 (HD44780) en modo 4 bits.
 * Pineado segun el documento de pineado del invernadero (Nano Master):
 *   RS -> D8   (PB0)
 *   EN -> D9   (PB1)
 *   D4 -> D10  (PB2)
 *   D5 -> D11  (PB3)
 *   D6 -> D12  (PB4)
 *   D7 -> D13  (PB5)
 *
 * Author : ferg7
 */
#ifndef LCD_H
#define LCD_H

#include <avr/io.h>
#include <stdint.h>

// LCD 16x2 en modo paralelo de 4 bits, con el pineado ya definido del
// proyecto: RS=D8, EN=D9, D4=D10, D5=D11, D6=D12, D7=D13. Los 6 pines
// caen todos en PORTB (D8-D13 = PB0-PB5 en el Nano).
#define LCD_DDR   DDRB
#define LCD_PORT  PORTB
#define LCD_RS    PB0
#define LCD_EN    PB1
#define LCD_D4    PB2
#define LCD_D5    PB3
#define LCD_D6    PB4
#define LCD_D7    PB5

void LCD_init(void);
void LCD_clear(void);
void LCD_set_cursor(uint8_t fila, uint8_t columna);
void LCD_print(const char *str);
void LCD_print_char(uint8_t caracter);
void LCD_print_int(int16_t num);
void LCD_clear_line(uint8_t fila);

#endif
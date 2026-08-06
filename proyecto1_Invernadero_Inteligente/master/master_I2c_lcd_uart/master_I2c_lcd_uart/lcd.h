

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

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

// Inicializa el LCD (modo 4 bits, 2 lineas, cursor apagado).
void LCD_init(void);

// Borra toda la pantalla y regresa el cursor a la posicion inicial.
void LCD_clear(void);

// Mueve el cursor a una fila (0 o 1) y columna (0-15).
void LCD_set_cursor(uint8_t fila, uint8_t columna);

// Imprime un string en la posicion actual del cursor, tal cual (sin rellenar
// el resto de la fila).
void LCD_print(const char *str);

// Imprime un numero sin signo en la posicion actual del cursor.
void LCD_print_uint(uint16_t valor);

// Imprime un numero con signo en la posicion actual del cursor.
void LCD_print_int(int16_t valor);

// Escribe un string completo en una fila (0 o 1), rellenando con espacios
// el resto de las 16 columnas. Util para que no queden caracteres viejos
// pegados cuando el nuevo texto es mas corto que el anterior.
void LCD_print_line(uint8_t fila, const char *str);

#endif /* LCD_H_ */
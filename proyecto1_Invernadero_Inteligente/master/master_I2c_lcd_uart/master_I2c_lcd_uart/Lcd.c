/*
 * lcd.c
 *
 * Author : ferg7
 */

#define F_CPU 16000000UL

// Fuerza la ruta "vieja" de _delay_ms()/_delay_us() (basada en
// _delay_loop_2, no en __builtin_avr_delay_cycles). Esa ruta no necesita
// que el valor del delay sea una constante de compilacion, asi que
// compila sin problema sin importar el nivel de optimizacion del
// proyecto (-O0, -Og, -Os, etc). Un poco menos precisa, pero para
// delays de milisegundos como los del LCD no se nota.
#define __DELAY_BACKWARD_COMPATIBLE__

#include "lcd.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

// Todos los pines del LCD caen en PORTB (D8-D13), asi que basta con un
// solo puerto para todo el driver.
#define LCD_DDR  DDRB
#define LCD_PORT PORTB

#define LCD_RS PB0 // D8
#define LCD_EN PB1 // D9
#define LCD_D4 PB2 // D10
#define LCD_D5 PB3 // D11
#define LCD_D6 PB4 // D12
#define LCD_D7 PB5 // D13

static void LCD_pulso_enable(void)
{
	LCD_PORT |= (1 << LCD_EN);
	_delay_us(1);
	LCD_PORT &= ~(1 << LCD_EN);
	_delay_us(100);
}

static void LCD_enviar_nibble(uint8_t nibble)
{
	if (nibble & 0x01) LCD_PORT |= (1 << LCD_D4); else LCD_PORT &= ~(1 << LCD_D4);
	if (nibble & 0x02) LCD_PORT |= (1 << LCD_D5); else LCD_PORT &= ~(1 << LCD_D5);
	if (nibble & 0x04) LCD_PORT |= (1 << LCD_D6); else LCD_PORT &= ~(1 << LCD_D6);
	if (nibble & 0x08) LCD_PORT |= (1 << LCD_D7); else LCD_PORT &= ~(1 << LCD_D7);
	LCD_pulso_enable();
}

// es_dato = 0 -> se envia como comando (RS=0)
// es_dato = 1 -> se envia como dato/caracter (RS=1)
static void LCD_enviar_byte(uint8_t valor, uint8_t es_dato)
{
	if (es_dato) LCD_PORT |= (1 << LCD_RS); else LCD_PORT &= ~(1 << LCD_RS);

	LCD_enviar_nibble(valor >> 4);
	LCD_enviar_nibble(valor & 0x0F);

	_delay_us(50);
}

static void LCD_comando(uint8_t comando)
{
	LCD_enviar_byte(comando, 0);

	// clear (0x01) y home (0x02) tardan mas que el resto de comandos
	if (comando == 0x01 || comando == 0x02)
	{
		_delay_ms(2);
	}
}

void LCD_init(void)
{
	LCD_DDR |= (1 << LCD_RS) | (1 << LCD_EN) | (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);
	LCD_PORT &= ~((1 << LCD_RS) | (1 << LCD_EN) | (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));

	_delay_ms(40); // espera de encendido, segun datasheet HD44780

	// Secuencia de "despertado" para forzar modo 4 bits, sin importar en
	// que estado haya quedado el LCD antes.
	LCD_enviar_nibble(0x03);
	_delay_ms(5);
	LCD_enviar_nibble(0x03);
	_delay_us(150);
	LCD_enviar_nibble(0x03);
	LCD_enviar_nibble(0x02); // aqui ya entra en modo 4 bits

	LCD_comando(0x28); // 4 bits, 2 lineas, fuente 5x8
	LCD_comando(0x0C); // display encendido, cursor y blink apagados
	LCD_comando(0x06); // el cursor avanza solo, sin desplazar la pantalla
	LCD_comando(0x01); // limpia pantalla
}

void LCD_clear(void)
{
	LCD_comando(0x01);
}

void LCD_set_cursor(uint8_t fila, uint8_t columna)
{
	// En un LCD 16x2, la fila 0 empieza en 0x00 y la fila 1 en 0x40 (asi
	// esta organizada la memoria DDRAM del HD44780, no es continua).
	uint8_t direccion = (fila == 0) ? 0x00 : 0x40;
	direccion += columna;
	LCD_comando(0x80 | direccion);
}

void LCD_print(const char *str)
{
	while (*str)
	{
		LCD_enviar_byte((uint8_t)(*str), 1);
		str++;
	}
}

void LCD_print_uint(uint16_t valor)
{
	char buffer[6];
	itoa(valor, buffer, 10);
	LCD_print(buffer);
}

void LCD_print_int(int16_t valor)
{
	char buffer[7];
	itoa(valor, buffer, 10);
	LCD_print(buffer);
}

void LCD_print_line(uint8_t fila, const char *str)
{
	LCD_set_cursor(fila, 0);

	uint8_t columna = 0;
	while (str[columna] != '\0' && columna < 16)
	{
		LCD_enviar_byte((uint8_t)str[columna], 1);
		columna++;
	}

	// Rellena lo que sobra con espacios, para que no queden pegados
	// caracteres de una lectura anterior mas larga.
	while (columna < 16)
	{
		LCD_enviar_byte(' ', 1);
		columna++;
	}
}
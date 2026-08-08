/*
 * lcd.c
 *
 * Author : ferg7
 */

#define F_CPU 16000000UL

#include "lcd.h"
#include <util/delay.h>

// Los pulsos de EN y los tiempos entre nibbles son del orden de
// microsegundos, algo mucho mas corto de lo que se puede medir con el
// millis() de 1ms del Timer0. Por eso aqui si se usa _delay_us/_delay_ms
// del compilador: no es un delay de logica de la aplicacion (como
// esperar a que un sensor este listo), es el tiempo minimo que pide el
// propio controlador del LCD para leer cada pulso.

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

static void LCD_enviar_byte(uint8_t valor, uint8_t es_dato)
{
	if (es_dato) LCD_PORT |= (1 << LCD_RS); else LCD_PORT &= ~(1 << LCD_RS);

	LCD_enviar_nibble(valor >> 4);   // nibble alto primero
	LCD_enviar_nibble(valor & 0x0F); // luego el nibble bajo
}

static void LCD_comando(uint8_t cmd)
{
	LCD_enviar_byte(cmd, 0);
}

static void LCD_dato(uint8_t dato)
{
	LCD_enviar_byte(dato, 1);
}

void LCD_init(void)
{
	LCD_DDR |= (1 << LCD_RS) | (1 << LCD_EN) | (1 << LCD_D4) |
	(1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);

	_delay_ms(50); // tiempo que pide el datasheet despues de energizar

	// Secuencia de "despertar" del HD44780: se manda el nibble 0x03 tres
	// veces con pausas especificas, es la forma estandar de forzar al
	// controlador a modo 4 bits sin importar en que estado haya quedado.
	LCD_enviar_nibble(0x03);
	_delay_ms(5);
	LCD_enviar_nibble(0x03);
	_delay_us(150);
	LCD_enviar_nibble(0x03);
	_delay_us(150);
	LCD_enviar_nibble(0x02); // ahora si, queda fijo en modo 4 bits

	LCD_comando(0x28); // 4 bits, 2 lineas, fuente 5x8
	LCD_comando(0x08); // pantalla apagada mientras se termina de configurar
	LCD_comando(0x01); // clear
	_delay_ms(2);       // el comando clear pide su propio tiempo aparte
	LCD_comando(0x06); // el cursor avanza a la derecha con cada caracter
	LCD_comando(0x0C); // pantalla encendida, cursor y parpadeo apagados
}

void LCD_clear(void)
{
	LCD_comando(0x01);
	_delay_ms(2);
}

void LCD_set_cursor(uint8_t fila, uint8_t columna)
{
	uint8_t direccion = (fila == 0) ? (0x80 + columna) : (0xC0 + columna);
	LCD_comando(direccion);
}

void LCD_print(const char *str)
{
	while (*str)
	{
		LCD_dato((uint8_t)(*str++));
	}
}

void LCD_print_char(uint8_t caracter)
{
	LCD_dato(caracter);
}

void LCD_print_int(int16_t num)
{
	char buffer[7];
	uint8_t i = 0;
	uint8_t negativo = 0;

	if (num < 0)
	{
		negativo = 1;
		num = -num;
	}

	if (num == 0)
	{
		LCD_dato('0');
		return;
	}

	while (num > 0)
	{
		buffer[i++] = (num % 10) + '0';
		num /= 10;
	}

	if (negativo)
	{
		LCD_dato('-');
	}

	while (i > 0)
	{
		LCD_dato(buffer[--i]);
	}
}

void LCD_clear_line(uint8_t fila)
{
	// Se sobreescribe la linea con espacios en vez de mandar el comando
	// de clear completo, porque ese comando borra las 2 lineas y aqui
	// solo se quiere refrescar una (la otra tiene el titulo/modo fijo).
	LCD_set_cursor(fila, 0);
	for (uint8_t i = 0; i < 16; i++)
	{
		LCD_dato(' ');
	}
	LCD_set_cursor(fila, 0);
}
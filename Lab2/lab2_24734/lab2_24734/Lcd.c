/*
 * Lcd.c
 *
 * Created: 16/07/2026 17:53:51
 *  Author: ferg7
 */ 

#include "lcd.h"
#include <util/delay.h>

// ---------------------------------------------------------------
// Funciones internas (no las necesita usar el usuario de la libreria,
// por eso son "static")
// ---------------------------------------------------------------

// Genera el pulso de Enable para que la LCD "capture" lo que dejamos en el bus.
// El datasheet pide un pulso minimo de unos cientos de nanosegundos, con 1us
// nos sobra margen de sobra a 16MHz.
static void LCD_PulseEnable(void)
{
	LCD_CTRL_PORT |= (1 << LCD_E);
	_delay_us(1);
	LCD_CTRL_PORT &= ~(1 << LCD_E);
	_delay_us(100);   // le damos tiempo a la LCD de procesar el dato antes de seguir
}

// Deja el byte completo en el bus de datos (como usamos PORTD entero, es una sola instruccion)
static void LCD_WriteByte(uint8_t valor)
{
	LCD_DATA_PORT = valor;
	LCD_PulseEnable();
}


void LCD_Command(uint8_t cmd)
{
	LCD_CTRL_PORT &= ~(1 << LCD_RS);   // RS = 0 -> lo que mandamos es un comando
	LCD_WriteByte(cmd);

	// Clear display (0x01) y Cursor home (0x02) son mas lentos que el resto de comandos
	if (cmd == 0x01 || cmd == 0x02)
	_delay_ms(2);
	else
	_delay_us(50);
}

void LCD_Data(uint8_t data)
{
	LCD_CTRL_PORT |= (1 << LCD_RS);    // RS = 1 -> lo que mandamos es un caracter a mostrar
	LCD_WriteByte(data);
	_delay_us(50);
}

void LCD_Init(void)
{
	// Todo PORTD como salida (bus de datos completo)
	LCD_DATA_DDR = 0xFF;

	// RS y E como salida, arrancamos ambos en bajo
	LCD_CTRL_DDR |= (1 << LCD_RS) | (1 << LCD_E);
	LCD_CTRL_PORT &= ~((1 << LCD_RS) | (1 << LCD_E));

	_delay_ms(50);   // power-on reset interno de la LCD (el datasheet pide minimo 15ms,
	// aqui le damos bastante mas margen mientras depuramos)

	// Secuencia de inicializacion en 8 bits segun el datasheet del HD44780.
	// Mandamos el Function Set tres veces seguidas: es la forma recomendada
	// por el fabricante para "resincronizar" la LCD si por algun motivo
	// quedo en un estado raro (por ejemplo si el micro se reinicio a medias
	// mientras la LCD ya estaba energizada).
	LCD_Command(0x38);
	_delay_ms(5);
	LCD_Command(0x38);
	_delay_ms(1);
	LCD_Command(0x38);
	_delay_ms(1);

	LCD_Command(0x38);   // Function set: bus 8 bits, 2 lineas, fuente 5x8
	LCD_Command(0x0C);   // Display ON, cursor OFF, sin parpadeo   00001100
	LCD_Command(0x01);   // Clear display
	_delay_ms(3);
	LCD_Command(0x06);   // Entry mode: cursor incrementa, sin desplazar la pantalla
}

void LCD_Clear(void)
{
	LCD_Command(0x01);
}

void LCD_SetCursor(uint8_t fila, uint8_t columna)
{
	// Direcciones de memoria DDRAM: la fila 0 arranca en 0x00 y la fila 1 en 0x40
	// (esto sale directo de la diapositiva de "Memoria DDRAM" del PDF de la clase)
	uint8_t direccion = (fila == 0) ? columna : (0x40 + columna);
	LCD_Command(0x80 | direccion);   // 0x80 = comando "Set DDRAM address"
}

void LCD_Print(const char *str)
{
	while (*str)
	{
		LCD_Data(*str);
		str++;
	}
}
/*
 * Lcd.c
 *
 * Created: 16/07/2026 17:53:51
 *  Author: ferg7
 */ 

#include "lcd.h"
#include <util/delay.h>



// Pequeño ayudante: prende o apaga un pin especifico segun un bit.
// Nos ahorra repetir el if/else ocho veces en LCD_WriteByte.
static void LCD_SetPin(volatile uint8_t *port, uint8_t pin, uint8_t encendido)
{
	if (encendido)
	*port |= (1 << pin);
	else
	*port &= ~(1 << pin);
}

// Genera el pulso de Enable para que la LCD capture lo que dejamos en el bus.
// El datasheet pide un pulso minimo de unos cientos de nanosegundos, con 1us
// nos sobra margen de sobra a 16MHz.
static void LCD_PulseEnable(void)
{
	LCD_E_PORT |= (1 << LCD_E_PIN);
	_delay_us(1);
	LCD_E_PORT &= ~(1 << LCD_E_PIN);
	_delay_us(100);   // le damos tiempo a la LCD de procesar el dato antes de seguir
}

// Escribe el byte completo en el bus de datos, pero ahora pin por pin,
// porque las 8 lineas ya no viven todas en el mismo puerto (D0 y D1
// quedaron en PORTB porque los pines originales del Arduino que usaban
// -D0 y D1- los necesita el UART; D2-D7 se quedaron donde siempre, en PORTD).
static void LCD_WriteByte(uint8_t valor)
{
	LCD_SetPin(&LCD_D0_PORT, LCD_D0_PIN, valor & 0x01);
	LCD_SetPin(&LCD_D1_PORT, LCD_D1_PIN, valor & 0x02);
	LCD_SetPin(&LCD_D2_PORT, LCD_D2_PIN, valor & 0x04);
	LCD_SetPin(&LCD_D3_PORT, LCD_D3_PIN, valor & 0x08);
	LCD_SetPin(&LCD_D4_PORT, LCD_D4_PIN, valor & 0x10);
	LCD_SetPin(&LCD_D5_PORT, LCD_D5_PIN, valor & 0x20);
	LCD_SetPin(&LCD_D6_PORT, LCD_D6_PIN, valor & 0x40);
	LCD_SetPin(&LCD_D7_PORT, LCD_D7_PIN, valor & 0x80);

	LCD_PulseEnable();
}



void LCD_Command(uint8_t cmd)
{
	LCD_RS_PORT &= ~(1 << LCD_RS_PIN);   // RS = 0  lo que mandamos es un comando
	LCD_WriteByte(cmd);

	// Clear display (0x01) y Cursor home (0x02) son mas lentos que el resto de comandos
	if (cmd == 0x01 || cmd == 0x02)
	_delay_ms(2);
	else
	_delay_us(50);
}

void LCD_Data(uint8_t data)
{
	LCD_RS_PORT |= (1 << LCD_RS_PIN);    // RS = 1  lo que mandamos es un caracter a mostrar
	LCD_WriteByte(data);
	_delay_us(50);
}

void LCD_Init(void)
{
	// Cada linea de datos como salida (ya no es un solo DDRD = 0xFF,
	// porque estan repartidas en dos puertos distintos)
	LCD_D0_DDR |= (1 << LCD_D0_PIN);
	LCD_D1_DDR |= (1 << LCD_D1_PIN);
	LCD_D2_DDR |= (1 << LCD_D2_PIN);
	LCD_D3_DDR |= (1 << LCD_D3_PIN);
	LCD_D4_DDR |= (1 << LCD_D4_PIN);
	LCD_D5_DDR |= (1 << LCD_D5_PIN);
	LCD_D6_DDR |= (1 << LCD_D6_PIN);
	LCD_D7_DDR |= (1 << LCD_D7_PIN);

	// RS y E como salida, arrancamos ambos en bajo
	LCD_RS_DDR |= (1 << LCD_RS_PIN);
	LCD_E_DDR  |= (1 << LCD_E_PIN);
	LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
	LCD_E_PORT  &= ~(1 << LCD_E_PIN);

	_delay_ms(50);   // power-on reset interno de la LCD (el datasheet pide alago minimo 15ms,
	// aqui le damos bastante mas margen mientras depuramos)

	// Secuencia de inicializacion acá en 8 bits segun el datasheet del HD44780.
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
	LCD_Command(0x0C);   // Display ON, cursor OFF, sin parpadeo
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
	//en base a información de documentación
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
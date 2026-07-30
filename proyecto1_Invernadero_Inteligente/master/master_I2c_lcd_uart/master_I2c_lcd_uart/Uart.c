/*
 * Uart.c
 *
 * Created: 30/07/2026 01:11:58
 *  Author: ferg7
 */ 
#include "uart.h"

void UART_init(void)
{
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * UART_BAUD)) - 1);

	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

	// Por ahora solo se necesita transmitir (imprimir resultados),
	// asi que unicamente se habilita TXEN0.
	UCSR0B = (1 << TXEN0);

	// Formato del frame: 8 bits de datos, sin paridad, 1 bit de stop.
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(char data)
{
	// Se espera a que el buffer de transmision quede vacio (UDRE0 = 1)
	// antes de meter un byte nuevo, si no se pisaria el que sigue en cola.
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void UART_print(const char *str)
{
	while (*str)
	{
		UART_transmit(*str++);
	}
}

void UART_print_uint(uint16_t num)
{
	char buffer[6];
	uint8_t i = 0;

	if (num == 0)
	{
		UART_transmit('0');
		return;
	}

	// Se van sacando los digitos de derecha a izquierda...
	while (num > 0)
	{
		buffer[i++] = (num % 10) + '0';
		num /= 10;
	}

	// ...y se imprimen en el orden correcto (de izquierda a derecha).
	while (i > 0)
	{
		UART_transmit(buffer[--i]);
	}
}

void UART_print_float(float num, uint8_t decimals)
{
	// Conversion manual a texto: se evita usar printf con floats porque
	// en AVR eso requiere enlazar una libreria extra (libprintf_flt) y
	// consume bastante memoria de programa para lo que necesitamos aqui.
	if (num < 0)
	{
		UART_transmit('-');
		num = -num;
	}

	uint16_t parte_entera = (uint16_t)num;
	float parte_decimal = num - parte_entera;

	UART_print_uint(parte_entera);
	UART_transmit('.');

	for (uint8_t i = 0; i < decimals; i++)
	{
		parte_decimal *= 10;
		uint8_t digito = (uint8_t)parte_decimal;
		UART_transmit(digito + '0');
		parte_decimal -= digito;
	}
}
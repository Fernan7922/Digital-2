/*
 * Uart.c
 *
 * Created: 30/07/2026 15:14:43
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "uart.h"

void UART_init(void)
{
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * UART_BAUD)) - 1);
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(char data)
{
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

void UART_print_int(int16_t num)
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
		UART_transmit('0');
		return;
	}

	while (num > 0)
	{
		buffer[i++] = (num % 10) + '0';
		num /= 10;
	}

	if (negativo)
	{
		UART_transmit('-');
	}

	while (i > 0)
	{
		UART_transmit(buffer[--i]);
	}
}
/*
 * Uart.c
 *
 * Created: 30/07/2026 01:11:58
 *  Author: ferg7
 */ 
#include "uart.h"
#include <avr/interrupt.h>
#define F_CPU 16000000UL

static volatile char rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_indice = 0;
static volatile uint8_t rx_linea_lista = 0;

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

void UART_print_uint(uint16_t num)
{
	char buffer[6];
	uint8_t i = 0;

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

	while (i > 0)
	{
		UART_transmit(buffer[--i]);
	}
}

void UART_print_float(float num, uint8_t decimals)
{
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

void UART_habilitar_recepcion(void)
{
	// RXEN0 prende el receptor, RXCIE0 hace que cada byte que llegue
	// dispare la interrupcion de abajo en vez de tener que estar
	// revisando el registro a cada rato desde el loop principal.
	UCSR0B |= (1 << RXEN0) | (1 << RXCIE0);
}

ISR(USART_RX_vect)
{
	char c = UDR0;

	if (c == '\n')
	{
		rx_buffer[rx_indice] = '\0';
		rx_linea_lista = 1;
		rx_indice = 0;
	}
	else if (c != '\r')
	{
		if (rx_indice < (UART_RX_BUFFER_SIZE - 1))
		{
			rx_buffer[rx_indice++] = c;
		}
	}
}

uint8_t UART_hay_linea_nueva(void)
{
	return rx_linea_lista;
}

const char *UART_obtener_linea(void)
{
	rx_linea_lista = 0;
	return (const char *)rx_buffer;
}
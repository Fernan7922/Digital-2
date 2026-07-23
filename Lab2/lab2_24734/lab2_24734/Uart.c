/*
 * Uart.c
 *
 * Created: 22/07/2026 23:35:46
 *  Author: ferg7
 */ 

// Librería recopilada de progras anteriores
#include "uart.h"

// En este caso la recepcion de caracteres (+ y -) no se toma como tal en esta libería.
// Este archivo solo prende el hardware del UART (incluyendo la interrupcion
// de recepcion). La logica de que hacer con cada caracter que llega vive
// en el ISR(USART_RX_vect) que esta en main.c, porque es ahi donde tiene
// sentido tener el  como tal el contador (la variable contador es parte del programa,
// no de esta libreria 

void UART_Init(uint32_t baudios)
{
	// Formula estandar del datasheet para calcular el registro UBRR
	// a partir de la velocidad que queremos (baudios)
	uint16_t ubrr = (F_CPU / (16UL * baudios)) - 1;

	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

	// TXEN0  = habilita transmision
	// RXEN0  = habilita recepcion
	// RXCIE0 = habilita la interrupcion "llego un dato nuevo" (para no
	//          tener que estar preguntando todo el tiempo si hay algo)
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

	// 8 bits de datos, sin paridad, 1 bit de stop (el formato "8N1" de toda la vida)
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_TransmitChar(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));  // esperamos a que el buffer de transmision este libre
	UDR0 = c;
}

void UART_TransmitString(const char *str)
{
	while (*str)
	{
		UART_TransmitChar(*str);
		str++;
	}
}
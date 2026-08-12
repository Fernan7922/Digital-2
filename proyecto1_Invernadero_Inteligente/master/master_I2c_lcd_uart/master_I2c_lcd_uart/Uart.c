/*
 * Uart.c
 *
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "uart.h"
#include <avr/interrupt.h>
#define F_CPU 16000000UL

// Buffer donde la interrupcion de recepcion va juntando los comandos
// que manda el ESP32 (MODO:AUTO, BOMBA:ON, etc), caracter por
// caracter, hasta encontrar el salto de linea.
#define TAM_BUFFER_RX 32
static volatile char buffer_rx[TAM_BUFFER_RX];
static volatile uint8_t indice_rx = 0;
static volatile uint8_t linea_lista = 0;

void UART_init(void)
{
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * UART_BAUD)) - 1);
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	// Ahora tambien se habilita la recepcion (antes solo se transmitia),
	// para poder recibir los comandos de modo/actuadores del ESP32.
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	// Formato del frame: 8 bits de datos, sin paridad, 1 bit de stop.
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Se dispara cada vez que llega un byte nuevo por UART. Se mantiene
// corta a proposito, como corresponde a una ISR.
ISR(USART_RX_vect)
{
	char c = UDR0;

	// Si todavia no se ha leido la linea anterior desde el loop
	// principal, se descartan los caracteres nuevos para no pisarla a
	// medias.
	if (linea_lista)
	{
		return;
	}

	if (c == '\n')
	{
		buffer_rx[indice_rx] = '\0';
		linea_lista = 1;
		indice_rx = 0;
	}
	else if (c != '\r')
	{
		if (indice_rx < (TAM_BUFFER_RX - 1))
		{
			buffer_rx[indice_rx++] = c;
		}
	}
}

uint8_t UART_hay_linea(void)
{
	return linea_lista;
}

void UART_leer_linea(char *destino, uint8_t tam_max)
{
	uint8_t i = 0;
	while (buffer_rx[i] != '\0' && i < (tam_max - 1))
	{
		destino[i] = buffer_rx[i];
		i++;
	}
	destino[i] = '\0';
	linea_lista = 0;
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
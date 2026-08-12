/*
 * Uart.h
 *
 * Created: 1/08/2026 02:01:11
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdint.h>

#define UART_BAUD 9600UL

void UART_init(void);
void UART_transmit(char data);
void UART_print(const char *str);
void UART_print_int(int16_t num);

#endif
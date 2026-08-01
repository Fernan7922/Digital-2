/*
 * Uart.h
 *
 * Created: 1/08/2026 02:01:11
 *  Author: ferg7
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
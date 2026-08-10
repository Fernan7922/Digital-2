/*
 * uart.h
 *
 * Author: ferg7
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

#define UART_BAUD 9600UL

void UART_init(void);
void UART_transmit(char data);
void UART_print(const char *str);
void UART_print_uint(uint16_t num);
void UART_print_float(float num, uint8_t decimals);

// Recepcion de comandos desde el ESP32. Va llenando un buffer por
// interrupcion en segundo plano; UART_hay_linea() avisa cuando ya
// junto una linea completa (hasta el '\n') y UART_leer_linea() la
// copia afuera, ya sin el '\n' ni el '\r'.
uint8_t UART_hay_linea(void);
void UART_leer_linea(char *destino, uint8_t tam_max);

#endif /* UART_H_ */
/*
 * Uart.h
 *
 * Created: 30/07/2026 01:12:15
 *  Author: ferg7
 */ 

#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdint.h>

#define UART_BAUD 9600UL
#define UART_RX_BUFFER_SIZE 32

void UART_init(void);
void UART_transmit(char data);
void UART_print(const char *str);
void UART_print_uint(uint16_t num);
void UART_print_float(float num, uint8_t decimals);

// Recepcion: se usa para escuchar los comandos que manda el ESP32 (los
// que vienen de Adafruit IO). Es por interrupcion, asi que no bloquea
// el loop principal esperando caracteres.
void UART_habilitar_recepcion(void);
uint8_t UART_hay_linea_nueva(void);
const char *UART_obtener_linea(void);

#endif
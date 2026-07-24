/*
 * uart.h
 *
 * Created: 23/07/2026 17:46:43
 *  Author: ferg7
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>

void UART_Init(uint32_t baudios);
void UART_TransmitChar(char c);
void UART_TransmitString(const char *str);




#endif /* UART_H_ */
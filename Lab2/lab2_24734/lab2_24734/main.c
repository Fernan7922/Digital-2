// Fernando Guzman
//24734
//MAIN
//Laboratorio 2   ADC y CONTADOR



#define F_CPU 16000000UL   // Nano corre a 16MHz, esto lo necesitan lcd.c y adc.c para calcular sus tiempos bien(ojó también hay que configurar la frecuencia no solo en progra, se vió que también hay 
//que configurarla en Microchip, en configuraciones.

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "adc.h"
#include <avr/interrupt.h>   // necesario para usar ISR() y sei()
#include "uart.h"             // la nueva librería que agregamos
volatile uint16_t contador = 0; //

//Esta funcion NO se llama nunca a mano: el hardware la ejecuta solo,
// automaticamente, cada vez que llega un byte nuevo por el UART.
// Aqui es donde revisamos si lo que llego fue '+' o '-
ISR(USART_RX_vect)
{
	char recibido = UDR0;   // leemos el caracter que acaba de llegar
	
	if (recibido == '+')
	{
		contador++;
	}
	else if (recibido == '-')
	{
		if (contador > 0)     // nunca lo dejamos bajar de 0
		contador--;
	}
}

int main(void)
{
	ADC_Init();
	LCD_Init();
	
	UART_Init(9600);
	sei();

	//  Pantalla de bienvenida, se muestra una sola vez al arrancar
	LCD_SetCursor(0, 5);
	LCD_Print("Lab 2");
	LCD_SetCursor(1, 1);
	LCD_Print("ADC y Contador");
	_delay_ms(2000);     // la dejamos un par de segundos en pantalla
	LCD_Clear();          // limpiamos antes de arrancar con las lecturas

	char buffer[8];   // suficiente para 1023 + el nulo del final
	char uart_msg[32];// se define así para contruir el mensaje completo que se manda por uart

	while (1)
	{
		uint16_t lectura = ADC_Read(0);   // Pot 1 esta en A0
		//ahora se cambiará para mostrar el valor de 1023 en voltaje
		//se trabajará en centésimas, esto evitará usar floats y sprintf com %f
		uint16_t voltios_x100 = (( uint32_t) lectura *500)/1023;
		uint8_t entero= voltios_x100 /100;
		uint8_t decimal= voltios_x100 %100;
		

		sprintf(buffer, "%d.%02dV", entero, decimal);  // lo dejamos tal cual, sin convertir a voltaje

		LCD_SetCursor(0, 1);
		LCD_Print("S1: ");
		LCD_SetCursor(1, 0);
		LCD_Print(buffer);
		LCD_Print("     ");   // espacios de sobra: 1023 tiene mas digitos que 0
		//Acá se usa la misma lógica para el S2
		uint16_t lectura2= ADC_Read(1);
		uint16_t voltios2_x100 = (( uint32_t) lectura2 *500)/1023;
		uint8_t entero2= voltios2_x100 /100;
		uint8_t decimal2= voltios2_x100 %100;
		

		sprintf(buffer, "%d.%02dV", entero2, decimal2);  // lo dejamos tal cual, sin convertir a voltaje

		LCD_SetCursor(0, 7);
		LCD_Print("S2: ");
		LCD_SetCursor(1, 6);
		LCD_Print(buffer);
		LCD_Print("     ");   // espacios de sobra: 1023 tiene mas
		
		
		
		
		// Etiqueta y valor de S3 (contador), en columna 11 de cada fila
		LCD_SetCursor(0, 12);
		LCD_Print("S3:");
		
		sprintf(buffer, "%u", contador);   // el "%u" es por contador es unsigned
		LCD_SetCursor(1, 13);
		LCD_Print(buffer);
		LCD_Print("  ");   // espacios de sobra por si el numero anterior tenia mas digitos
		
		// ---- Enviar ambas lecturas por UART hacia la PC ----
		sprintf(uart_msg, "S1:%d.%02dV S2:%d.%02dV\r\n", entero, decimal, entero2, decimal2);
		UART_TransmitString(uart_msg);
		
		_delay_ms(200);
		}
}
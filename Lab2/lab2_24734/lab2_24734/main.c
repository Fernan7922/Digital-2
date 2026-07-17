#define F_CPU 16000000UL   // Nano corre a 16MHz, esto lo necesitan lcd.c y adc.c para calcular sus tiempos

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "adc.h"

int main(void)
{
	ADC_Init();
	LCD_Init();

	// ---- Pantalla de bienvenida, se muestra una sola vez al arrancar ----
	LCD_SetCursor(0, 0);
	LCD_Print("Lab 2");
	LCD_SetCursor(1, 0);
	LCD_Print("ADC y Contador");
	_delay_ms(2000);     // la dejamos un par de segundos en pantalla
	LCD_Clear();          // limpiamos antes de arrancar con las lecturas

	char buffer[8];   // suficiente para "1023" + el nulo del final

	while (1)
	{
		uint16_t lectura = ADC_Read(0);   // Pot 1 esta en A0

		sprintf(buffer, "%u", lectura);   // lo dejamos tal cual, sin convertir a voltaje

		LCD_SetCursor(0, 0);
		LCD_Print("S1: ");
		LCD_SetCursor(1, 0);
		LCD_Print(buffer);
		LCD_Print("     ");   // espacios de sobra: 1023 tiene mas digitos que 0

		_delay_ms(200);      // pausa corta para que el valor no ande parpadeando sin parar
	}
}
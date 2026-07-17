/*
 * lab2_24734.c
 *
 * Created: 16/07/2026 17:32:14
 * Author : ferg7
 */ 

#define F_CPU 16000000UL  // Nano corre a 16MHz, esto lo necesitan lcd.c y adc.c para calcular sus tiempos

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "adc.h"

int main(void)
{
	ADC_Init();
	LCD_Init();

	char buffer[8];   // suficiente para algo como "5.00 V" o "1023" + el nulo del final

	while (1)
	{
		// Por ahora solo tenemos un pot conectado (A0), asi que lo leemos una vez
		// y con ese mismo dato armamos las dos formas de visualizacion.
		// Cuando conecte el segundo pot para la Parte 2, este ADC_Read(1) en la
		// fila de abajo se cambia por su propia lectura en el canal correspondiente.
		uint16_t lectura = ADC_Read(0);

		// Fila 0: formato voltaje, en centesimas para no depender de floats ----
		uint16_t voltios_x100 = ((uint32_t)lectura * 500) / 1023;
		uint8_t entero  = voltios_x100 / 100;
		uint8_t decimal = voltios_x100 % 100;

		sprintf(buffer, "%d.%02d V", entero, decimal);

		LCD_SetCursor(0, 0);
		LCD_Print("S1: ");
		LCD_Print(buffer);
		LCD_Print("   ");   // espacios de sobra por si el numero anterior era mas largo

		// ---- Fila 1: mismo dato pero en decimal crudo (0-1023) ----
		sprintf(buffer, "%u", lectura);

		LCD_SetCursor(1, 0);
		LCD_Print("S2: ");
		LCD_Print(buffer);
		LCD_Print("     ");   // espacios de sobra: 1023 tiene mas digitos que 0

		_delay_ms(200);      // pausa corta para que el valor no ande parpadeando sin parar
	}
}

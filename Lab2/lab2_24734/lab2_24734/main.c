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
	LCD_SetCursor(0, 5);
	LCD_Print("Lab 2");
	LCD_SetCursor(1, 1);
	LCD_Print("ADC y Contador");
	_delay_ms(2000);     // la dejamos un par de segundos en pantalla
	LCD_Clear();          // limpiamos antes de arrancar con las lecturas

	char buffer[8];   // suficiente para 1023 + el nulo del final

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

		_delay_ms(200);      // pausa corta para que el valor no ande parpadeando sin parar
	}
}
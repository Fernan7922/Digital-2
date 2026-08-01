/*
 * master_I2c_lcd_uart.c
 *
 * Created: 30/07/2026 01:09:07
 * Author : ferg7
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "twi.h"
#include "uart.h"
#include "timer.h"
#include "aht10.h"
#include "servo.h"
#include "led.h"
#include "relay_fan.h"

// CALIBRACION DE LA VENTILACION - AJUSTAR 
// =====================================================================
// Se usan 2 umbrales (histeresis) igual que en el riego y la sombra:
// hace falta subir hasta TEMP_UMBRAL_ACTIVAR para que se active, y bajar
// hasta TEMP_UMBRAL_DESACTIVAR para que se apague. Asi no esta abriendo
// y cerrando la ventana en cada lectura si la temperatura anda pegada
// al limite.
#define TEMP_UMBRAL_ACTIVAR    30.0f
#define TEMP_UMBRAL_DESACTIVAR 29.0f
// =====================================================================

int main(void)
{
	// Orden de inicializacion: primero los perifericos que no dependen de
	// interrupciones (UART, TWI), luego los que si usan timers/ISR, y
	// hasta el final se habilitan las interrupciones globales con sei().
	UART_init();
	TWI_init();
	Timer_init();
	Servo_init();
	LED_init();
	Relay_fan_init();
	sei();

	UART_print("Iniciando AHT10...\r\n");
	AHT10_init();

	uint8_t ventilacion_activa = 0;

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000; // cada 2 segundos

	while (1)
	{
		// En vez de un delay bloqueante, se compara contra millis().
		if ((millis() - ultima_lectura) >= intervalo_lectura)
		{
			float temperatura, humedad;

			if (AHT10_read(&temperatura, &humedad))
			{
				UART_print("Temp: ");
				UART_print_float(temperatura, 1);
				UART_print(" C  Hum: ");
				UART_print_float(humedad, 1);
				UART_print(" %");

				// Del AHT10 aqui solo nos interesa la temperatura para
				// decidir la ventilacion (la humedad se sigue mostrando
				// por UART nada mas para verificar que el sensor funciona).
				if (!ventilacion_activa && temperatura > TEMP_UMBRAL_ACTIVAR)
				{
					Servo_set_angle(SERVO_ANGULO_ABIERTO);
					LED_on();
					Relay_fan_on();
					ventilacion_activa = 1;
					UART_print("  -> Ventilacion ACTIVADA");
				}
				else if (ventilacion_activa && temperatura < TEMP_UMBRAL_DESACTIVAR)
				{
					Servo_set_angle(SERVO_ANGULO_REPOSO);
					LED_off();
					Relay_fan_off();
					ventilacion_activa = 0;
					UART_print("  -> Ventilacion DESACTIVADA");
				}

				UART_print("\r\n");
			}
			else
			{
				UART_print("Sensor ocupado, se reintenta...\r\n");
			}

			ultima_lectura = millis();
		}
	}

	return 0;
}
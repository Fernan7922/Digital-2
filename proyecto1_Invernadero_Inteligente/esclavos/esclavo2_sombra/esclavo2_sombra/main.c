/*
 * - Nano Periferico 2 (Clima / Sombra)
 *
 * Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 *
 * Este nodo mide el nivel de luz con el LDR y controla el stepper que
 * despliega o retrae la malla de sombra. Por defecto decide solo (modo
 * automatico, con la histeresis del LDR de siempre); si el Master le
 * manda un comando de encender o apagar por I2C, pasa a obedecer ese
 * comando en vez de decidir con el sensor (modo manual, controlado
 * desde Adafruit IO). Reporta el nivel de luz y el estado de la sombra
 * al Master cada vez que este lo pide por I2C.
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"
#include "ldr_sensor.h"
#include "stepper.h"
#include "led.h"
#include "uart.h"
#include "timer.h"
#include "twi_slave.h"

#define INTERVALO_LECTURA_MS 1000UL

// Direccion I2C de este nodo, la que quedo asignada en el documento de
// pineado para el periferico de clima.
#define DIRECCION_I2C_PROPIA 0x09

// Comandos que puede mandar el Master por I2C. AUTO le devuelve el
// control al sensor local; ENCENDER/APAGAR son ordenes directas que
// solo se obedecen mientras dure el modo manual.
#define CMD_ACTUADOR_APAGAR 0x00
#define CMD_ACTUADOR_ENCENDER 0x01
#define CMD_ACTUADOR_AUTO 0x02

int main(void)
{
	ADC_init();
	LED_init();
	Stepper_init();
	UART_init();
	Timer_init();
	TWI_slave_init(DIRECCION_I2C_PROPIA);
	sei();
	UART_print("Periferico 2 - Sombra listo\r\n");
	
	uint8_t sombra_activa = 0;
	uint32_t ultima_lectura = millis();

	// Arranca en automatico (modo_manual = 0) hasta que llegue una
	// orden del Master que diga lo contrario.
	uint8_t modo_manual = 0;
	while (1)
	{
		uint32_t ahora = millis();

		// Se revisa en cada vuelta del loop (no solo cuando toca leer
		// el sensor) para que un cambio de modo o una orden manual se
		// apliquen al toque.
		if (TWI_slave_hay_comando())
		{
			uint8_t comando = TWI_slave_leer_comando();
			if (comando == CMD_ACTUADOR_AUTO)
			{
				// Si se venia de manual, se retrae la sombra antes de
				// devolver el control al sensor, para no dejarla
				// desplegada arrastrando un estado viejo.
				if (modo_manual)
				{
					Stepper_set_objetivo(0);
					LED_off();
					sombra_activa = 0;
					UART_print("Modo AUTOMATICO\r\n");
				}
				modo_manual = 0;
			}
			else if (comando == CMD_ACTUADOR_ENCENDER || comando == CMD_ACTUADOR_APAGAR)
			{
				modo_manual = 1;
				sombra_activa = (comando == CMD_ACTUADOR_ENCENDER);
				if (sombra_activa)
				{
					Stepper_set_objetivo(STEPPER_PASOS_SOMBRA);
					LED_on();
					UART_print("Modo MANUAL - Sombra ACTIVADA\r\n");
				}
				else
				{
					Stepper_set_objetivo(0);
					LED_off();
					sombra_activa = 0;
					UART_print("Modo MANUAL - Sombra DESACTIVADA\r\n");
				}
			}
		}
		if ((ahora - ultima_lectura) >= INTERVALO_LECTURA_MS)
		{
			ultima_lectura = ahora;
			uint16_t valor = LDR_read_raw();
			UART_print("LDR: ");
			UART_print_int((int16_t)valor);

			// La logica automatica de siempre (histeresis del LDR),
			// pero solo corre si no estamos en modo manual.
			if (!modo_manual)
			{
				uint8_t necesita_sombra = LDR_necesita_sombra(valor, sombra_activa);
				if (necesita_sombra && !sombra_activa)
				{
					Stepper_set_objetivo(STEPPER_PASOS_SOMBRA);
					LED_on();
					sombra_activa = 1;
					UART_print(" -> Sombra ACTIVADA\r\n");
				}
				else if (!necesita_sombra && sombra_activa)
				{
					Stepper_set_objetivo(0);
					LED_off();
					sombra_activa = 0;
					UART_print(" -> Sombra DESACTIVADA\r\n");
				}
				else
				{
					UART_print("\r\n");
				}
			}
			else
			{
				UART_print("\r\n");
			}

			// El paquete que lee el Master: nivel de luz crudo (partido
			// en 2 bytes porque el ADC llega hasta 1023 y no cabe en
			// uno solo), y el estado real de la sombra.
			uint8_t paquete[3];
			paquete[0] = (uint8_t)(valor >> 8);
			paquete[1] = (uint8_t)(valor & 0xFF);
			paquete[2] = sombra_activa;
			TWI_slave_set_buffer(paquete, 3);
		}
		Stepper_update(ahora);
	}
	return 0;
}
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
#define DIRECCION_I2C_PROPIA 0x09

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
	uint8_t modo_manual = 0;

	while (1)
	{
		uint32_t ahora = millis();

		if (TWI_slave_hay_comando())
		{
			uint8_t comando = TWI_slave_leer_comando();

			if (comando == CMD_ACTUADOR_AUTO)
			{
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
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"
#include "soil_sensor.h"
#include "relay.h"
#include "led.h"
#include "uart.h"
#include "timer.h"
#include "twi_slave.h"

#define UMBRAL_HUMEDAD 40
#define DURACION_RIEGO_MS 5000UL
#define ENFRIAMIENTO_MS 10000UL
#define INTERVALO_LECTURA_MS 1000UL
#define DIRECCION_I2C_PROPIA 0x08

#define CMD_ACTUADOR_APAGAR 0x00
#define CMD_ACTUADOR_ENCENDER 0x01
#define CMD_ACTUADOR_AUTO 0x02

typedef enum
{
	ESTADO_IDLE,
	ESTADO_REGANDO
} estado_riego_t;

int main(void)
{
	ADC_init();
	Relay_init();
	LED_init();
	UART_init();
	Timer_init();
	TWI_slave_init(DIRECCION_I2C_PROPIA);
	sei();
	UART_print("Periferico 1 - Riego listo\r\n");
	
	estado_riego_t estado = ESTADO_IDLE;
	uint32_t ultima_lectura = millis();
	uint32_t inicio_riego = 0;
	uint32_t fin_enfriamiento = 0;

	uint8_t modo_manual = 0;
	uint8_t bomba_manual_on = 0;

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
					Relay_off();
					LED_off();
					estado = ESTADO_IDLE;
					fin_enfriamiento = 0;
					UART_print("Modo AUTOMATICO\r\n");
				}
				modo_manual = 0;
			}
			else if (comando == CMD_ACTUADOR_ENCENDER || comando == CMD_ACTUADOR_APAGAR)
			{
				modo_manual = 1;
				bomba_manual_on = (comando == CMD_ACTUADOR_ENCENDER);

				if (bomba_manual_on)
				{
					Relay_on();
					LED_on();
					UART_print("Modo MANUAL - Bomba ENCENDIDA\r\n");
				}
				else
				{
					Relay_off();
					LED_off();
					UART_print("Modo MANUAL - Bomba APAGADA\r\n");
				}
			}
		}

		if ((ahora - ultima_lectura) >= INTERVALO_LECTURA_MS)
		{
			ultima_lectura = ahora;
			uint16_t crudo = Soil_read_raw();
			int16_t porcentaje = Soil_read_percent();
			UART_print("Crudo: ");
			UART_print_int((int16_t)crudo);
			UART_print(" | Humedad: ");
			UART_print_int(porcentaje);
			UART_print("%\r\n");

			if (!modo_manual)
			{
				if (estado == ESTADO_IDLE &&
				porcentaje < UMBRAL_HUMEDAD &&
				ahora >= fin_enfriamiento)
				{
					Relay_on();
					LED_on();
					inicio_riego = ahora;
					estado = ESTADO_REGANDO;
					UART_print(">> Bomba ENCENDIDA\r\n");
				}
			}

			uint8_t paquete[2];
			paquete[0] = (uint8_t)porcentaje;
			paquete[1] = modo_manual ? bomba_manual_on : (estado == ESTADO_REGANDO);
			TWI_slave_set_buffer(paquete, 2);
		}

		if (!modo_manual && estado == ESTADO_REGANDO)
		{
			if ((ahora - inicio_riego) >= DURACION_RIEGO_MS)
			{
				Relay_off();
				LED_off();
				fin_enfriamiento = ahora + ENFRIAMIENTO_MS;
				estado = ESTADO_IDLE;
				UART_print(">> Bomba APAGADA\r\n");
				
				uint8_t paquete[2];
				paquete[0] = (uint8_t)Soil_read_percent();
				paquete[1] = 0;
				TWI_slave_set_buffer(paquete, 2);
			}
		}
	}
	return 0;
}
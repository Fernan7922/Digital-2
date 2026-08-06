/*
 * esclavo1_riego.c
 *
 * Created: 30/07/2026 15:08:13
 * Author : ferg7
 */ 

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

// Direccion I2C de este nodo como esclavo, la misma que quedo asignada
// en el documento de pineado para el periferico de riego.
#define DIRECCION_I2C_PROPIA 0x08

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

	// El TWI esclavo se inicializa antes de sei() (igual que el resto
	// de perifericos que usan interrupciones), asi el bus ya esta listo
	// para contestar apenas se habiliten las interrupciones globales.
	TWI_slave_init(DIRECCION_I2C_PROPIA);

	sei();

	UART_print("Periferico 1 - Riego listo\r\n");

	estado_riego_t estado = ESTADO_IDLE;
	uint32_t ultima_lectura = millis();
	uint32_t inicio_riego = 0;
	uint32_t fin_enfriamiento = 0;

	while (1)
	{
		uint32_t ahora = millis();

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

			// Se arma el paquete que el master va a poder leer la proxima
			// vez que pregunte por este nodo: humedad de suelo (0-100) y
			// el estado de la bomba en ese instante. Se manda aqui, junto
			// con la lectura, para que el dato que vea el master siempre
			// vaya emparejado (humedad y estado tomados al mismo tiempo).
			uint8_t paquete[2];
			paquete[0] = (uint8_t)porcentaje;
			paquete[1] = (estado == ESTADO_REGANDO) ? 1 : 0;
			TWI_slave_set_buffer(paquete, 2);
		}

		if (estado == ESTADO_REGANDO)
		{
			if ((ahora - inicio_riego) >= DURACION_RIEGO_MS)
			{
				Relay_off();
				LED_off();
				fin_enfriamiento = ahora + ENFRIAMIENTO_MS;
				estado = ESTADO_IDLE;
				UART_print(">> Bomba APAGADA\r\n");

				// El estado cambio fuera del bloque de lectura periodica
				// (la bomba se apaga por tiempo, no porque se haya vuelto
				// a leer el sensor), asi que el paquete se actualiza aqui
				// tambien para que el master no seentere tarde de que ya
				// se apago.
				uint8_t paquete[2];
				paquete[0] = (uint8_t)Soil_read_percent();
				paquete[1] = 0;
				TWI_slave_set_buffer(paquete, 2);
			}
		}
	}

	return 0;
}
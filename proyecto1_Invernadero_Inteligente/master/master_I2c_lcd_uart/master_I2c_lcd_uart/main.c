/*
 * master_I2c_lcd_uart.c
 *
 * Created: 30/07/2026 01:09:07
 * Author : ferg7
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "twi.h"
#include "uart.h"
#include "timer.h"
#include "aht10.h"
#include "servo.h"
#include "led.h"
#include "relay_fan.h"
#include "lcd.h"

#define DIR_PERIFERICO_RIEGO 0x08
#define DIR_PERIFERICO_CLIMA 0x09

#define TEMP_UMBRAL_ACTIVAR    30.0f
#define TEMP_UMBRAL_DESACTIVAR 28.0f

// Modos de operacion. En AUTO, el master calcula todo con sus propios
// umbrales, como ya funcionaba. En MANUAL, ignora esos umbrales y usa lo
// ultimo que le haya llegado del ESP32 (que a su vez viene de Adafruit).
typedef enum
{
	MODO_AUTO = 0,
	MODO_MANUAL = 1
} modo_t;

static modo_t modo_actual = MODO_AUTO;

// Ultimo valor pedido manualmente para cada actuador (solo se usan
// cuando modo_actual == MODO_MANUAL).
static uint8_t manual_bomba = 0;
static uint8_t manual_servo = 0;
static uint8_t manual_sombra = 0;

// Espera basada en millis(), igual que en el resto del proyecto, para
// las pantallas de arranque (no es una espera de sensor, pero tampoco
// hay nada mas que hacer mientras se muestran, asi que aqui si se
// espera de corrido en vez de armar una maquina de estados para esto).
static void wait_boot(uint32_t ms)
{
	uint32_t inicio = millis();
	while ((millis() - inicio) < ms);
}

static void mostrar_boot(void)
{
	LCD_set_cursor(0, 0);
	LCD_print("Invernadero");
	LCD_set_cursor(1, 0);
	LCD_print("Digital 2");
	wait_boot(1500);

	LCD_clear();
	LCD_set_cursor(0, 0);
	LCD_print("Fernando");
	wait_boot(1500);

	LCD_clear();
	LCD_set_cursor(0, 0);
	LCD_print("Chichu");
	wait_boot(1500);

	LCD_clear();
}

// Se lee una linea que mando el ESP32 (algo tipo "MODO:1", "BOMBA:1",
// "SERVO:0", "SOMBRA:1") y se actualiza la variable que corresponda.
// El formato es a proposito igual de simple que el que ya usa el ESP32
// para mandarnos sus propias lineas.
static void procesar_comando_esp32(const char *linea)
{
	if (strncmp(linea, "MODO:", 5) == 0)
	{
		modo_actual = (linea[5] == '1') ? MODO_MANUAL : MODO_AUTO;
		UART_print("Modo cambiado por Adafruit\r\n");
	}
	else if (strncmp(linea, "BOMBA:", 6) == 0)
	{
		manual_bomba = (linea[6] == '1') ? 1 : 0;
	}
	else if (strncmp(linea, "SERVO:", 6) == 0)
	{
		manual_servo = (linea[6] == '1') ? 1 : 0;
	}
	else if (strncmp(linea, "SOMBRA:", 7) == 0)
	{
		manual_sombra = (linea[7] == '1') ? 1 : 0;
	}
}

int main(void)
{
	UART_init();
	UART_habilitar_recepcion();
	TWI_init();
	Timer_init();
	Servo_init();
	LED_init();
	Relay_fan_init();
	LCD_init();
	sei();

	mostrar_boot();

	UART_print("Iniciando AHT10...\r\n");
	AHT10_init();

	uint8_t ventilacion_activa = 0;
	uint8_t bomba_activa = 0;
	uint8_t sombra_activa = 0;

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000;

	while (1)
	{
		// Los comandos del ESP32 se revisan en cada vuelta del loop, no
		// solo cada 2s como las lecturas: si alguien cambia el modo o
		// un actuador desde Adafuit, se quiere que reaccione al toque,
		// no que espere a la siguiente ronda de sensores.
		if (UART_hay_linea_nueva())
		{
			procesar_comando_esp32(UART_obtener_linea());
		}

		if ((millis() - ultima_lectura) >= intervalo_lectura)
		{
			ultima_lectura = millis();

			float temperatura = 0;
			float humedad_aht10 = 0;
			uint8_t temp_ok = AHT10_read(&temperatura, &humedad_aht10);

			uint8_t datos_riego[1];
			uint8_t riego_ok = TWI_read_from_slave(DIR_PERIFERICO_RIEGO, datos_riego, 1);
			uint8_t humedad_suelo = datos_riego[0];

			uint8_t datos_clima[2];
			uint8_t clima_ok = TWI_read_from_slave(DIR_PERIFERICO_CLIMA, datos_clima, 2);
			uint16_t nivel_luz = ((uint16_t)datos_clima[0] << 8) | datos_clima[1];

			// --- Aqui se decide que hacer con cada actuador ---
			if (modo_actual == MODO_AUTO)
			{
				if (temp_ok)
				{
					if (!ventilacion_activa && temperatura > TEMP_UMBRAL_ACTIVAR)
					{
						ventilacion_activa = 1;
					}
					else if (ventilacion_activa && temperatura < TEMP_UMBRAL_DESACTIVAR)
					{
						ventilacion_activa = 0;
					}
				}

				if (riego_ok)
				{
					if (!bomba_activa && humedad_suelo < 40)
					{
						bomba_activa = 1;
					}
					else if (bomba_activa && humedad_suelo >= 45)
					{
						bomba_activa = 0;
					}
				}

				if (clima_ok)
				{
					if (!sombra_activa && nivel_luz > 600)
					{
						sombra_activa = 1;
					}
					else if (sombra_activa && nivel_luz < 550)
					{
						sombra_activa = 0;
					}
				}
			}
			else // MODO_MANUAL: se ignoran los sensores para decidir,
			// se usa directamente lo que llego de Adafruit.
			{
				ventilacion_activa = manual_servo;
				bomba_activa = manual_bomba;
				sombra_activa = manual_sombra;
			}

			// --- El servo/led/rele del master se controlan directo ---
			if (ventilacion_activa)
			{
				Servo_set_angle(SERVO_ANGULO_ABIERTO);
				LED_on();
				Relay_fan_on();
			}
			else
			{
				Servo_set_angle(SERVO_ANGULO_REPOSO);
				LED_off();
				Relay_fan_off();
			}

			// --- Bomba y sombra se mandan por I2C, los ejecuta el esclavo ---
			TWI_write_to_slave(DIR_PERIFERICO_RIEGO, bomba_activa);
			TWI_write_to_slave(DIR_PERIFERICO_CLIMA, sombra_activa);

			// --- Reporte por UART (lo sigue leyendo el ESP32 igual que antes) ---
			UART_print("Temp: ");
			if (temp_ok) { UART_print_float(temperatura, 1); UART_print(" C"); }
			else { UART_print("ERROR"); }

			UART_print("  Humedad suelo: ");
			if (riego_ok) { UART_print_uint(humedad_suelo); UART_print("%"); }
			else { UART_print("SIN RESPUESTA"); }

			UART_print("  LDR: ");
			if (clima_ok) { UART_print_uint(nivel_luz); }
			else { UART_print("SIN RESPUESTA"); }
			UART_print("\r\n");

			// --- LCD: fila de arriba muestra el modo, abajo los valores ---
			LCD_clear_line(0);
			LCD_print(modo_actual == MODO_AUTO ? "Modo: AUTO" : "Modo: MANUAL");

			LCD_clear_line(1);
			LCD_print("T:");
			LCD_print_int(temp_ok ? (int16_t)temperatura : 0);
			LCD_print_char(0xDF); // codigo del simbolo de grado en el charset del LCD
			LCD_print(" H:");
			LCD_print_int(riego_ok ? humedad_suelo : 0);
			LCD_print(" L:");
			LCD_print_int(clima_ok ? (int16_t)nivel_luz : 0);
		}
	}

	return 0;
}
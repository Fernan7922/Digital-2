/*
 * master_I2c_lcd_uart.c
 *
 * Created: 30/07/2026 01:09:07
 * Author : ferg7
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "twi.h"
#include "uart.h"
#include "timer.h"
#include "aht10.h"
#include "servo.h"
#include "led.h"
#include "relay_fan.h"
#include "lcd.h"

// Direcciones I2C de los 2 Nano esclavos, en base al pineado del proyecto.
#define DIR_PERIFERICO_RIEGO 0x08
#define DIR_PERIFERICO_CLIMA 0x09

// El periferico de riego manda 2 bytes: humedad de suelo (0-100) y el
// estado de la bomba (0 apagada, 1 encendida).
#define LONGITUD_PAQUETE_RIEGO 2

// El periferico de clima manda 3 bytes: los 2 bytes del valor crudo del
// LDR (no cabe en 1 solo byte porque el ADC da 0-1023) y el estado de
// la sombra (0 retraida, 1 desplegada).
#define LONGITUD_PAQUETE_CLIMA 3

#define TEMP_UMBRAL_ACTIVAR    30.0f
#define TEMP_UMBRAL_DESACTIVAR 28.0f

// Muestra una pantalla fija por cierto tiempo y luego borra, para armar
// la secuencia de carga inicial (splash) antes de empezar a leer sensores.
static void Splash_mostrar(const char *linea1, const char *linea2, uint16_t duracion_ms)
{
	LCD_print_line(0, linea1);
	LCD_print_line(1, linea2);
	_delay_ms(duracion_ms);
	LCD_clear();
	_delay_ms(300); // pausa corta con la pantalla en blanco entre cuadro y cuadro
}

// Arma la fila de abajo del LCD con los 3 valores que ya se mandan por UART.
// Formato compacto (sin ° ni %) porque en 16 columnas no cabe todo con
// las unidades: "T:24 H:60 L:512"
static void LCD_actualizar_sensores(uint8_t temp_ok, float temperatura,
                                     uint8_t riego_ok, uint8_t humedad_suelo,
                                     uint8_t clima_ok, uint16_t nivel_luz)
{
	char linea2[17];
	char temp_str[5];
	char hum_str[5];
	char luz_str[6];

	if (temp_ok)
	{
		itoa((int)(temperatura + 0.5f), temp_str, 10);
	}
	else
	{
		strcpy(temp_str, "--");
	}

	if (riego_ok)
	{
		itoa(humedad_suelo, hum_str, 10);
	}
	else
	{
		strcpy(hum_str, "--");
	}

	if (clima_ok)
	{
		itoa(nivel_luz, luz_str, 10);
	}
	else
	{
		strcpy(luz_str, "----");
	}

	strcpy(linea2, "T:");
	strcat(linea2, temp_str);
	strcat(linea2, " H:");
	strcat(linea2, hum_str);
	strcat(linea2, " L:");
	strcat(linea2, luz_str);

	LCD_print_line(1, linea2);
}

int main(void)
{
	UART_init();
	TWI_init();
	Timer_init();
	Servo_init();
	LED_init();
	Relay_fan_init();
	LCD_init();
	sei();

	UART_print("Iniciando AHT10...\r\n");
	AHT10_init();

	// --- Pantalla de carga (splash) mientras arranca todo ---
	// Nota de FerG: ajusta textos/tiempos aqui si no es exactamente lo
	// que querias, quedo como mejor interpretacion del mensaje de voz.
	Splash_mostrar("Invernadero", "BE3029", 1500);
	Splash_mostrar("Fernando", "", 1000);
	Splash_mostrar("Chichu", "", 1000);

	// La fila de arriba ya no cambia despues del splash, asi que se deja
	// escrita una sola vez antes de entrar al loop.
	LCD_print_line(0, "Invernadero");

	uint8_t ventilacion_activa = 0;

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000; // cada 2 segundos

	while (1)
	{
		if ((millis() - ultima_lectura) >= intervalo_lectura)
		{
			ultima_lectura = millis();

			// --- Sensor propio del master (I2C directo, con twi.c) ---
			float temperatura = 0;
			float humedad_aht10 = 0; // se lee pero no se muestra: la
			// humedad que nos interesa mostrar
			// es la del suelo (capacitivo), no
			// esta del AHT10.
			uint8_t temp_ok = AHT10_read(&temperatura, &humedad_aht10);

			// --- Periferico de riego, via I2C como esclavo ---
			uint8_t datos_riego[LONGITUD_PAQUETE_RIEGO];
			uint8_t riego_ok = TWI_read_from_slave(DIR_PERIFERICO_RIEGO, datos_riego, LONGITUD_PAQUETE_RIEGO);
			uint8_t humedad_suelo = datos_riego[0];
			uint8_t bomba_activa = datos_riego[1];

			// --- Periferico de clima, via I2C como esclavo ---
			uint8_t datos_clima[LONGITUD_PAQUETE_CLIMA];
			uint8_t clima_ok = TWI_read_from_slave(DIR_PERIFERICO_CLIMA, datos_clima, LONGITUD_PAQUETE_CLIMA);
			uint16_t nivel_luz = ((uint16_t)datos_clima[0] << 8) | datos_clima[1];
			uint8_t sombra_activa = datos_clima[2];

			// --- Reporte por UART (se deja igual, para cuando se mande al ESP32) ---
			UART_print("Temp: ");
			if (temp_ok)
			{
				UART_print_float(temperatura, 1);
				UART_print(" C");
			}
			else
			{
				UART_print("ERROR");
			}

			UART_print("  Humedad suelo: ");
			if (riego_ok)
			{
				UART_print_uint(humedad_suelo);
				UART_print("%");
			}
			else
			{
				UART_print("SIN RESPUESTA");
			}

			UART_print("  LDR: ");
			if (clima_ok)
			{
				UART_print_uint(nivel_luz);
			}
			else
			{
				UART_print("SIN RESPUESTA");
			}
			UART_print("\r\n");

			// Las advertencias se separan del reporte de arriba para que
			// sea facil ubicarlas de un vistazo al ver el monitor serie.
			if (riego_ok && bomba_activa)
			{
				UART_print("  [AVISO] Bomba de riego ACTIVA\r\n");
			}
			if (clima_ok && sombra_activa)
			{
				UART_print("  [AVISO] Malla de sombra DESPLEGADA\r\n");
			}
			if (!riego_ok)
			{
				UART_print("  [AVISO] Periferico de riego no contesto\r\n");
			}
			if (!clima_ok)
			{
				UART_print("  [AVISO] Periferico de clima no contesto\r\n");
			}
			if (!temp_ok)
			{
				UART_print("  [AVISO] AHT10 no contesto o seguia ocupado\r\n");
			}

			// --- Reporte en LCD (fila de abajo, la de arriba ya quedo fija) ---
			LCD_actualizar_sensores(temp_ok, temperatura, riego_ok, humedad_suelo, clima_ok, nivel_luz);

			// --- Ventilacion, con la temperatura propia del master ---
			if (temp_ok)
			{
				if (!ventilacion_activa && temperatura > TEMP_UMBRAL_ACTIVAR)
				{
					Servo_set_angle(SERVO_ANGULO_ABIERTO);
					LED_on();
					Relay_fan_on();
					ventilacion_activa = 1;
					UART_print("  [AVISO] Ventilacion ACTIVADA\r\n");
				}
				else if (ventilacion_activa && temperatura < TEMP_UMBRAL_DESACTIVAR)
				{
					Servo_set_angle(SERVO_ANGULO_REPOSO);
					LED_off();
					Relay_fan_off();
					ventilacion_activa = 0;
					UART_print("  [AVISO] Ventilacion DESACTIVADA\r\n");
				}
			}
		}
	}

	return 0;
}
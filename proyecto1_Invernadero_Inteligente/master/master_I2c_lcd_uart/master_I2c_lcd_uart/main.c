#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h> // Necesario para las pausas de estabilización del bus I2C
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

#define LONGITUD_PAQUETE_RIEGO 2
#define LONGITUD_PAQUETE_CLIMA 3

#define TEMP_UMBRAL_ACTIVAR    30.0f
#define TEMP_UMBRAL_DESACTIVAR 28.0f

// Comandos I2C para los periféricos
#define CMD_ACTUADOR_APAGAR   0x00
#define CMD_ACTUADOR_ENCENDER 0x01
#define CMD_ACTUADOR_AUTO     0x02

#define TAM_BUFFER_RX_COMANDO 32

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
	LCD_set_cursor(0, 0);
	LCD_print("Invernadero");
}

// Escritura simple por I2C con protección de timeouts para evitar congelamientos
static uint8_t I2C_enviar_comando(uint8_t direccion, uint8_t comando)
{
	uint32_t timeout;
	const uint32_t MAX_TIMEOUT = 10000; // Límite de ciclos de reloj para declarar falla

	// 1. Enviar condición de START
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	timeout = 0;
	while (!(TWCR & (1 << TWINT)))
	{
		if (++timeout > MAX_TIMEOUT) {
			TWCR = 0; // Desactiva TWI para liberar el hardware
			TWCR = (1 << TWEN);
			return 0; // Aborta transmisión por timeout
		}
	}
	if ((TWSR & 0xF8) != 0x08) return 0;

	// 2. Enviar dirección del esclavo + bit de escritura
	TWDR = (direccion << 1);
	TWCR = (1 << TWINT) | (1 << TWEN);
	timeout = 0;
	while (!(TWCR & (1 << TWINT)))
	{
		if (++timeout > MAX_TIMEOUT) {
			TWCR = 0;
			TWCR = (1 << TWEN);
			return 0;
		}
	}
	if ((TWSR & 0xF8) != 0x18)
	{
		TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
		return 0;
	}

	// 3. Enviar el comando
	TWDR = comando;
	TWCR = (1 << TWINT) | (1 << TWEN);
	timeout = 0;
	while (!(TWCR & (1 << TWINT)))
	{
		if (++timeout > MAX_TIMEOUT) {
			TWCR = 0;
			TWCR = (1 << TWEN);
			return 0;
		}
	}
	uint8_t confirmado = ((TWSR & 0xF8) == 0x28);

	// 4. Enviar condición de STOP
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	_delay_ms(1); // Breve espera física para asentar la línea de bus

	return confirmado;
}

// Transmite los estados actuales de forma segura espaciando los envíos
static void transmitir_estados_I2C(uint8_t modo_manual, uint8_t cmd_bomba, uint8_t cmd_sombra)
{
	uint8_t comando_para_bomba = modo_manual
	? (cmd_bomba ? CMD_ACTUADOR_ENCENDER : CMD_ACTUADOR_APAGAR)
	: CMD_ACTUADOR_AUTO;
	uint8_t comando_para_sombra = modo_manual
	? (cmd_sombra ? CMD_ACTUADOR_ENCENDER : CMD_ACTUADOR_APAGAR)
	: CMD_ACTUADOR_AUTO;

	I2C_enviar_comando(DIR_PERIFERICO_RIEGO, comando_para_bomba);
	
	_delay_ms(10); // Pausa de estabilización eléctrica para evitar colisiones
	
	I2C_enviar_comando(DIR_PERIFERICO_CLIMA, comando_para_sombra);
}

// Procesa la línea de comando recibida por UART
static void procesarComando(const char *linea, uint8_t *modo_manual,
uint8_t *cmd_bomba, uint8_t *cmd_sombra,
uint8_t *cmd_ventilacion)
{
	if (strcmp(linea, "MODO:MANUAL") == 0) *modo_manual = 1;
	else if (strcmp(linea, "MODO:AUTO") == 0) *modo_manual = 0;
	else if (strcmp(linea, "BOMBA:ON") == 0) *cmd_bomba = 1;
	else if (strcmp(linea, "BOMBA:OFF") == 0) *cmd_bomba = 0;
	else if (strcmp(linea, "SOMBRA:ON") == 0) *cmd_sombra = 1;
	else if (strcmp(linea, "SOMBRA:OFF") == 0) *cmd_sombra = 0;
	else if (strcmp(linea, "VENT:ON") == 0) *cmd_ventilacion = 1;
	else if (strcmp(linea, "VENT:OFF") == 0) *cmd_ventilacion = 0;
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

	mostrar_boot();

	UART_print("Iniciando AHT10...\r\n");
	AHT10_init();

	uint8_t ventilacion_activa = 0;

	uint8_t modo_manual = 0;
	uint8_t cmd_bomba = 0;
	uint8_t cmd_sombra = 0;
	uint8_t cmd_ventilacion = 0;

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000;

	while (1)
	{
		if (UART_hay_linea())
		{
			char linea_comando[TAM_BUFFER_RX_COMANDO];
			UART_leer_linea(linea_comando, sizeof(linea_comando));
			procesarComando(linea_comando, &modo_manual, &cmd_bomba, &cmd_sombra, &cmd_ventilacion);
			
			// Envío instantáneo tras procesar comando
			transmitir_estados_I2C(modo_manual, cmd_bomba, cmd_sombra);
		}

		if ((millis() - ultima_lectura) >= intervalo_lectura)
		{
			ultima_lectura = millis();

			// --- Sensor propio del master (AHT10) ---
			float temperatura = 0;
			float humedad_aht10 = 0;
			uint8_t temp_ok = AHT10_read(&temperatura, &humedad_aht10);

			// --- Periférico de riego ---
			uint8_t datos_riego[LONGITUD_PAQUETE_RIEGO];
			uint8_t riego_ok = TWI_read_from_slave(DIR_PERIFERICO_RIEGO, datos_riego, LONGITUD_PAQUETE_RIEGO);
			uint8_t humedad_suelo = datos_riego[0];
			uint8_t bomba_activa = datos_riego[1];

			// --- Periférico de clima ---
			uint8_t datos_clima[LONGITUD_PAQUETE_CLIMA];
			uint8_t clima_ok = TWI_read_from_slave(DIR_PERIFERICO_CLIMA, datos_clima, LONGITUD_PAQUETE_CLIMA);
			uint16_t nivel_luz = ((uint16_t)datos_clima[0] << 8) | datos_clima[1];
			uint8_t sombra_activa = datos_clima[2];

			// --- Reporte por UART ---
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

			if (riego_ok && bomba_activa)
			{
				UART_print("  [AVISO] Bomba de riego ACTIVA\r\n");
			}
			if (clima_ok && sombra_activa)
			{
				UART_print("  [AVISO] Malla de sombra DESPLEGADA\r\n");
			}

			// --- Ventilacion ---
			if (modo_manual)
			{
				if (cmd_ventilacion && !ventilacion_activa)
				{
					Servo_set_angle(SERVO_ANGULO_ABIERTO);
					LED_on();
					Relay_fan_on();
					ventilacion_activa = 1;
					UART_print("  [AVISO] Ventilacion ACTIVADA (manual)\r\n");
				}
				else if (!cmd_ventilacion && ventilacion_activa)
				{
					Servo_set_angle(SERVO_ANGULO_REPOSO);
					LED_off();
					Relay_fan_off();
					ventilacion_activa = 0;
					UART_print("  [AVISO] Ventilacion DESACTIVADA (manual)\r\n");
				}
			}
			else if (temp_ok)
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

			// Envío periódico de respaldo
			transmitir_estados_I2C(modo_manual, cmd_bomba, cmd_sombra);

			// --- Pantalla LCD ---
			char linea2[30];
			char numero_temp[7];

			strcpy(linea2, "T:");
			itoa(temp_ok ? (int16_t)temperatura : 0, numero_temp, 10);
			strcat(linea2, numero_temp);
			strcat(linea2, "\xDF");

			strcat(linea2, " H:");
			itoa(riego_ok ? humedad_suelo : 0, numero_temp, 10);
			strcat(linea2, numero_temp);

			strcat(linea2, " L:");
			itoa(clima_ok ? (int16_t)nivel_luz : 0, numero_temp, 10);
			strcat(linea2, numero_temp);

			LCD_print_line(1, linea2);
		}
	}

	return 0;
}
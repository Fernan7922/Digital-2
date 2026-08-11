/*
 *  Nano Master
 *
 * Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 *
 * Este es el nodo maestro del invernadero: lee su propio sensor AHT10
 * (temperatura), le pregunta por I2C a los 2 Nano perifericos (humedad
 * de suelo y nivel de luz), decide si hay que regar, dar sombra o
 * ventilar (o deja que Adafruit IO lo decida en modo manual), y muestra
 * todo en el LCD y por UART hacia el ESP32.
 */

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

// Direcciones I2C de los 2 Nano esclavos, las mismas que quedaron
// asignadas en el documento de pineado del proyecto.
#define DIR_PERIFERICO_RIEGO 0x08
#define DIR_PERIFERICO_CLIMA 0x09

// Cuantos bytes manda cada periferico cuando el master lo lee.
#define LONGITUD_PAQUETE_RIEGO 2
#define LONGITUD_PAQUETE_CLIMA 3

// Umbrales de temperatura para la ventilacion en modo automatico.
// Se usan 2 (no 1 solo) para tener histeresis: hace falta subir hasta
// el de activar para prender, y bajar hasta el de desactivar para
// apagar, asi no esta prendiendo y apagando en cada lectura si la
// temperatura anda pegada al limite.
#define TEMP_UMBRAL_ACTIVAR    30.0f
#define TEMP_UMBRAL_DESACTIVAR 28.0f

// Comandos que se le mandan a cada periferico por I2C. AUTO le dice al
// periferico que siga decidiendo solo con su propio sensor (asi el modo
// automatico no cambia en nada de como ya funcionaba antes); ENCENDER y
// APAGAR son ordenes directas que solo se obedecen cuando el modo
// manual esta activo.
#define CMD_ACTUADOR_APAGAR   0x00
#define CMD_ACTUADOR_ENCENDER 0x01
#define CMD_ACTUADOR_AUTO     0x02

#define TAM_BUFFER_RX_COMANDO 32

// Espera basada en el timer, para las pantallas de arranque del LCD.
// No es una espera de sensor, pero mientras se muestran esas pantallas
// no hay nada mas que hacer, asi que aqui si se deja correr de corrido.
static void wait_boot(uint32_t ms)
{
	uint32_t inicio = millis();
	while ((millis() - inicio) < ms);
}

// Secuencia de arranque del LCD: se presenta el proyecto y el equipo
// antes de pasar a la pantalla de monitoreo normal.
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

// Manda 1 solo comando (direccion + 1 byte) a un periferico, siguiendo
// la misma secuencia START -> direccion -> dato -> STOP de siempre, pero
// con un contador de timeout en cada paso: si el bus se queda pegado
// esperando TWINT (por ejemplo porque el periferico se desconecto o
// esta reiniciandose), se suelta el TWI y se regresa "fallo" en vez de
// dejar el programa colgado ahi para siempre.
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

// Arma el comando que le toca a cada actuador segun el modo actual, y
// lo manda a los 2 perifericos. En automatico se manda CMD_ACTUADOR_AUTO
// (el periferico usa su propio sensor); en manual se manda ENCENDER o
// APAGAR segun lo que haya llegado de Adafruit. Se deja una pausa entre
// un periferico y otro porque mandar los 2 comandos pegados, uno justo
// despues del otro, le da menos tiempo al bus de asentarse entre
// transacciones.
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

// Traduce la linea de texto que manda el ESP32 (que a su vez viene de
// los botones de Adafruit IO) a las variables de modo y de cada
// actuador. El formato es sencillo a proposito: "ETIQUETA:VALOR".
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
	// Se inicializan primero los perifericos que no dependen de
	// interrupciones, y hasta el final se habilitan con sei().
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

	// Variables de modo y de cada actuador manual. Arrancan en
	// automatico y todo apagado, hasta que llegue una orden de Adafruit.
	uint8_t modo_manual = 0;
	uint8_t cmd_bomba = 0;
	uint8_t cmd_sombra = 0;
	uint8_t cmd_ventilacion = 0;

	uint32_t ultima_lectura = millis();
	const uint32_t intervalo_lectura = 2000;

	while (1)
	{
		// Los comandos de Adafruit se revisan en cada vuelta del loop
		// (no solo cada 2s), para que un cambio de modo o de un
		// actuador se aplique de inmediato y no hasta la siguiente
		// ronda de lecturas.
		if (UART_hay_linea())
		{
			char linea_comando[TAM_BUFFER_RX_COMANDO];
			UART_leer_linea(linea_comando, sizeof(linea_comando));
			procesarComando(linea_comando, &modo_manual, &cmd_bomba, &cmd_sombra, &cmd_ventilacion);
			
			// En cuanto se procesa el comando se manda de una vez a los
			// perifericos, sin esperar al envio periodico de mas abajo.
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
			// Este es el texto que el ESP32 lee del otro lado y sube a
			// Adafruit IO, por eso el formato ("Temp: ... Humedad
			// suelo: ... LDR: ...") no se puede cambiar libremente.
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

			// Avisos aparte del reporte principal, para que sea facil
			// ubicarlos de un vistazo en el monitor serie.
			if (riego_ok && bomba_activa)
			{
				UART_print("  [AVISO] Bomba de riego ACTIVA\r\n");
			}
			if (clima_ok && sombra_activa)
			{
				UART_print("  [AVISO] Malla de sombra DESPLEGADA\r\n");
			}

			// --- Ventilacion ---
			// En manual, la orden de Adafruit manda directo. En
			// automatico, se usa la temperatura del AHT10 con los 2
			// umbrales de arriba (histeresis), igual que siempre.
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

			// Se vuelve a mandar el estado a los perifericos aqui
			// tambien (ademas de cuando llega un comando nuevo), como
			// respaldo por si algun periferico se perdio el envio
			// anterior (por ejemplo si se reinicio a la mitad).
			transmitir_estados_I2C(modo_manual, cmd_bomba, cmd_sombra);

			// --- Pantalla LCD ---
			// Se arma toda la fila de abajo en un buffer y se manda de
			// una sola vez, para que LCD_print_line rellene con
			// espacios lo que sobre y no queden caracteres de una
			// lectura anterior mas larga pegados al final.
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
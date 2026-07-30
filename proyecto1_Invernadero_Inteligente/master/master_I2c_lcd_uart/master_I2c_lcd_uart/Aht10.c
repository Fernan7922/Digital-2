/*
 * Aht10.c
 *
 * Created: 30/07/2026 01:13:20
 *  Author: ferg7
 */ 

#include "aht10.h"
#include "twi.h"
#include "timer.h"

uint8_t AHT10_init(void)
{
	// Segun el datasheet, el sensor necesita un momento para arrancar
	// despues de energizarse antes de aceptar comandos.
	wait_ms(40);

	TWI_start();
	TWI_write((AHT10_ADDRESS << 1) | 0); // Direccion + bit de escritura (0)

	// Comando de inicializacion/calibracion del AHT10 (0xE1), con sus
	// 2 bytes de parametro fijos segun el datasheet.
	TWI_write(0xE1);
	TWI_write(0x08);
	TWI_write(0x00);

	TWI_stop();

	// Se le da tiempo al sensor de aplicar la calibracion interna.
	wait_ms(10);

	return 1;
}

uint8_t AHT10_read(float *temperatura, float *humedad)
{
	uint8_t datos[6];

	// Paso 1: se dispara una medicion nueva con el comando 0xAC.
	TWI_start();
	TWI_write((AHT10_ADDRESS << 1) | 0);
	TWI_write(0xAC);
	TWI_write(0x33);
	TWI_write(0x00);
	TWI_stop();

	// Paso 2: el sensor necesita minimo 75ms para tener el dato listo;
	// se espera un poco mas (80ms) por margen.
	wait_ms(80);

	// Paso 3: se vuelve a direccionar el sensor, ahora para leer 6 bytes.
	TWI_start();
	TWI_write((AHT10_ADDRESS << 1) | 1); // Direccion + bit de lectura (1)

	datos[0] = TWI_read_ack();  // Byte de estado
	datos[1] = TWI_read_ack();  // Humedad bits [19:12]
	datos[2] = TWI_read_ack();  // Humedad bits [11:4]
	datos[3] = TWI_read_ack();  // Humedad bits [3:0] + Temperatura bits [19:16]
	datos[4] = TWI_read_ack();  // Temperatura bits [15:8]
	datos[5] = TWI_read_nack(); // Temperatura bits [7:0] -> ultimo byte, NACK

	TWI_stop();

	// Si el bit 7 del byte de estado sigue en 1, el sensor seguia ocupado
	// midiendo y estos datos no son confiables todavia.
	if (datos[0] & 0x80)
	{
		return 0;
	}

	// Se arman los 20 bits crudos de humedad juntando los 3 bytes correspondientes.
	uint32_t humedad_cruda = ((uint32_t)datos[1] << 12) |
	((uint32_t)datos[2] << 4) |
	(datos[3] >> 4);

	// Los 4 bits bajos de datos[3] son en realidad la parte alta de la
	// temperatura, por eso se combinan con datos[4] y datos[5].
	uint32_t temp_cruda = ((uint32_t)(datos[3] & 0x0F) << 16) |
	((uint32_t)datos[4] << 8) |
	datos[5];

	// Formulas de conversion del datasheet del AHT10.
	*humedad = ((float)humedad_cruda / 1048576.0f) * 100.0f;
	*temperatura = (((float)temp_cruda / 1048576.0f) * 200.0f) - 50.0f;

	return 1;
}
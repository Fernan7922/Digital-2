/*
 * spi_slave.c
 *
 * Author: ferg7
 */
#include "spi_slave.h"
#include <avr/interrupt.h>

// ---------------------------------------------------------------
// Pinout SPI del ATmega328p en modo ESCLAVO (tabla "SPI Pin
// Overrides" del datasheet):
//
//   PB5 = D13 = SCK   -> entrada (el reloj lo controla el maestro)
//   PB4 = D12 = MISO  -> de direccion definida por el usuario -> salida
//                        (por aqui el esclavo contesta)
//   PB3 = D11 = MOSI  -> entrada (por aqui llega lo que manda el maestro)
//   PB2 = D10 = SS    -> entrada (el maestro la baja para seleccionarnos)
// ---------------------------------------------------------------

// Trama de 4 bytes que se va mandando: [pot1_alto, pot1_bajo, pot2_alto, pot2_bajo]
static volatile uint8_t trama[4] = {0, 0, 0, 0};
static volatile uint8_t indice = 0;

void SPI_Slave_Init(void)
{
	// Solo MISO es salida en modo esclavo; el resto las controla el maestro
	DDRB |= (1 << PB4);
	DDRB &= ~((1 << PB3) | (1 << PB5) | (1 << PB2));

	// Paso 1 del funcionamiento SPI (segun el PDF): dejamos el primer
	// byte ya cargado en SPDR, listo para la primera vez que el
	// maestro empiece a generar el reloj.
	SPDR = trama[0];

	// --- Registro SPCR ---
	// Bit 7 - SPIE : habilita la interrupcion "se completo una
	//                transferencia de 8 bits" (se dispara cuando
	//                SPIF se pone en 1 en SPSR)
	// Bit 6 - SPE  : habilita el modulo SPI
	// (No se pone MSTR porque el modulo queda en modo ESCLAVO;
	//  CPOL=0, CPHA=0 por defecto  Modo SPI 0, igual que el maestro)
	SPCR = (1 << SPE) | (1 << SPIE);

	// sei() (interrupciones globales) se activa una sola vez en el main
}

void SPI_Slave_ActualizarTrama(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
	// Solo actualizamos si el indice esta en 0 (recien se termino de
	// mandar la trama anterior por completo). Asi evitamos que una
	// trama se corrompa a la mitad por estar mezclando datos viejos
	// y nuevos mientras el maestro la esta leyendo.
	if (indice == 0)
	{
		trama[0] = b0;
		trama[1] = b1;
		trama[2] = b2;
		trama[3] = b3;
	}
}

// El hardware pone en 1 el bit SPIF del registro SPSR cada vez que
// termina de desplazar 8 bits completos ). Como
// tenemos SPIE=1, eso dispara automaticamente esta interrupcion.
ISR(SPI_STC_vect)
{
	indice++;
	if (indice >= 4)
	{
		indice = 0;
	}
	// Dejamos listo el siguiente byte en SPDR (Paso 1 de nuevo) para
	// que salga en la proxima transferencia que pida el maestro.
	SPDR = trama[indice];
}
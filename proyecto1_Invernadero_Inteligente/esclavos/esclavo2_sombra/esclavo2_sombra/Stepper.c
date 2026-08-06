/*
 * Stepper.c
 *
 * Created: 1/08/2026 01:57:47
 *  Author: ferg7
 */ 
#include "stepper.h"

// Secuencia de medio paso (half-step) para el 28BYJ-48 via ULN2003.
// Cada fila son los 4 pines IN1, IN2, IN3, IN4 en ese orden. Usar medio
// paso da mas torque y un giro mas suave que la secuencia de paso completo.
static const uint8_t secuencia[8][4] = {
	{1, 0, 0, 0},
	{1, 1, 0, 0},
	{0, 1, 0, 0},
	{0, 1, 1, 0},
	{0, 0, 1, 0},
	{0, 0, 1, 1},
	{0, 0, 0, 1},
	{1, 0, 0, 1}
};

static int8_t indice_secuencia = 0;
static int32_t posicion_actual = 0;
static int32_t posicion_objetivo = 0;
static uint32_t ultimo_paso_ms = 0;

static void aplicar_paso(uint8_t idx)
{
	if (secuencia[idx][0]) STEPPER_PORT |= (1 << STEPPER_IN1); else STEPPER_PORT &= ~(1 << STEPPER_IN1);
	if (secuencia[idx][1]) STEPPER_PORT |= (1 << STEPPER_IN2); else STEPPER_PORT &= ~(1 << STEPPER_IN2);
	if (secuencia[idx][2]) STEPPER_PORT |= (1 << STEPPER_IN3); else STEPPER_PORT &= ~(1 << STEPPER_IN3);
	if (secuencia[idx][3]) STEPPER_PORT |= (1 << STEPPER_IN4); else STEPPER_PORT &= ~(1 << STEPPER_IN4);
}

static void apagar_bobinas(void)
{
	// Con el motor detenido no hace falta mantener ninguna bobina energizada;
	// apagarlas evita que el driver y el motor se calienten sin necesidad.
	STEPPER_PORT &= ~((1 << STEPPER_IN1) | (1 << STEPPER_IN2) | (1 << STEPPER_IN3) | (1 << STEPPER_IN4));
}

void Stepper_init(void)
{
	STEPPER_DDR |= (1 << STEPPER_IN1) | (1 << STEPPER_IN2) | (1 << STEPPER_IN3) | (1 << STEPPER_IN4);
	apagar_bobinas();
}

void Stepper_set_objetivo(int32_t objetivo_pasos)
{
	posicion_objetivo = objetivo_pasos;
}

void Stepper_update(uint32_t ahora_ms)
{
	// Ya se llego a donde se necesitaba, no hay nada que mover.
	if (posicion_actual == posicion_objetivo)
	{
		return;
	}

	// Todavia no toca dar el siguiente paso.
	if ((ahora_ms - ultimo_paso_ms) < STEPPER_INTERVALO_MS)
	{
		return;
	}

	ultimo_paso_ms = ahora_ms;

	if (posicion_actual < posicion_objetivo)
	{
		// Hay que desplegar sombra: se avanza en la secuencia.
		indice_secuencia++;
		if (indice_secuencia > 7) indice_secuencia = 0;
		posicion_actual++;
	}
	else
	{
		// Hay que regresar a reposo: se retrocede en la secuencia.
		indice_secuencia--;
		if (indice_secuencia < 0) indice_secuencia = 7;
		posicion_actual--;
	}

	aplicar_paso((uint8_t)indice_secuencia);

	// Si con este paso ya se llego al objetivo, se apagan las bobinas.
	if (posicion_actual == posicion_objetivo)
	{
		apagar_bobinas();
	}
}

uint8_t Stepper_esta_en_movimiento(void)
{
	return (posicion_actual != posicion_objetivo);
}

int32_t Stepper_posicion_actual(void)
{
	return posicion_actual;
}
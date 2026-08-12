/*
 * Servo.c
 *
 * Created: 1/08/2026 02:59:26
 *    Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "servo.h"
#include <avr/interrupt.h>

// El servo necesita un pulso cada 20ms, con un ancho de 1-2ms segun el
// angulo deseado. Se usa el Timer1 (16 bits) en modo CTC con 2 registros
// de comparacion:
//   OCR1A = marca el final del periodo completo (20ms)  ahi se prende
//           el pin y el timer se reinicia solo (por ser el TOP del CTC).
//   OCR1B = marca cuando ya paso el ancho de pulso que toca -> ahi se
//           apaga el pin.
// Con prescaler de 8 y F_CPU=16MHz, cada cuenta del timer dura 0.5us.

#define SERVO_TICKS_POR_US 2UL   // 1 / 0.5us
#define SERVO_PERIODO_TICKS 39999UL // 20000us * 2 - 1

void Servo_init(void)
{
	SERVO_DDR |= (1 << SERVO_PIN);

	TCCR1A = 0; // no se usan las salidas automaticas de Timer1, todo a mano
	TCCR1B = (1 << WGM12) | (1 << CS11); // CTC, TOP=OCR1A, prescaler 8

	OCR1A = (uint16_t)SERVO_PERIODO_TICKS;

	TIMSK1 = (1 << OCIE1A) | (1 << OCIE1B);

	// Arranca en la posicion de reposo calibrada, no en 0 grados.
	Servo_set_angle(SERVO_ANGULO_REPOSO);
}

void Servo_set_angle(uint8_t angulo_grados)
{
	if (angulo_grados > 180)
	{
		angulo_grados = 180;
	}

	uint16_t pulso_us = SERVO_PULSO_MIN_US +
	((uint32_t)angulo_grados * (SERVO_PULSO_MAX_US - SERVO_PULSO_MIN_US)) / 180;

	uint16_t ticks = (uint16_t)(pulso_us * SERVO_TICKS_POR_US);

	// OCR1B es de 16 bits; en un AVR de 8 bits esa escritura no es atomica,
	// asi que se protege un instante para que el ISR no la lea a la mitad.
	cli();
	OCR1B = ticks;
	sei();
}

// Empieza el pulso: se prende el pin justo cuando el timer se reinicia
// (arranca un ciclo nuevo de 20ms).
ISR(TIMER1_COMPA_vect)
{
	SERVO_PORT |= (1 << SERVO_PIN);
}

// Termina el pulso: se apaga el pin cuando ya paso el ancho que le toca
// segun el angulo actual.
ISR(TIMER1_COMPB_vect)
{
	SERVO_PORT &= ~(1 << SERVO_PIN);
}
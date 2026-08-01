/*
 * Servo.h
 *
 * Created: 1/08/2026 02:58:00
 *  Author: ferg7
 */ 


#ifndef SERVO_H
#define SERVO_H

#include <avr/io.h>
#include <stdint.h>

// Pin de señal del servo: D4 = PD4. No es un pin de PWM de hardware,
// pero no importa: el pulso se genera a mano con el Timer1 y 2
// interrupciones (arranca el pulso, lo apaga a los X microsegundos).
#define SERVO_DDR  DDRD
#define SERVO_PORT PORTD
#define SERVO_PIN  PD4

// =====================================================================
// CALIBRACION DEL SERVO - AJUSTAR CON LA VENTANA YA MONTADA
// =====================================================================
// Ancho de pulso para 0 y 180 grados. La mayoria de los SG90 responden
// bien entre 1000us y 2000us, pero algunos llegan un poco mas lejos
// (500-2400us). Si el servo no llega a girar lo que se espera, o hace
// ruido raro al llegar al limite, se debe ajustar estos valores.
#define SERVO_PULSO_MIN_US 800
#define SERVO_PULSO_MAX_US 2500

// Posicion en la que arranca el servo al encender el sistema . Poner el angulo que corresponda a "ventana cerrada"
// segun como quede montado el brazo del servo en la ventila.
#define SERVO_ANGULO_REPOSO   10

// Posicion a la que se mueve cuando hace falta ventilar.
#define SERVO_ANGULO_ABIERTO  120
// =====================================================================

void Servo_init(void);
void Servo_set_angle(uint8_t angulo_grados);

#endif
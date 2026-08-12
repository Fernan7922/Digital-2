/*
 * Stepper.h
 *
 * Created: 1/08/2026 01:56:52
*  Proyecto Invernadero - BE3029 Electronica Digital 2
* Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 


#ifndef STEPPER_H
#define STEPPER_H

#include <avr/io.h>
#include <stdint.h>

// Pines hacia el ULN2003 (IN1-IN4): D2, D3, D4, D5.
#define STEPPER_DDR   DDRD
#define STEPPER_PORT  PORTD
#define STEPPER_IN1   PD2
#define STEPPER_IN2   PD3
#define STEPPER_IN3   PD4
#define STEPPER_IN4   PD5

// =====================================================================
// CALIBRACION DEL STEPPER - 
// =====================================================================
// PASOS_SOMBRA: cuantos pasos tiene que girar el motor para que la
// cortina/malla quede completamente desplegada. Empieza con un numero
// chico (por ejemplo 200) y ve subiendolo hasta que el recorrido cubra
// justo lo que necesitas en tu maqueta. Si se pasa de largo o no llega,
// solo cambia este numero, no hay que tocar el resto del codigo.
#define STEPPER_PASOS_SOMBRA 3000

// STEPPER_INTERVALO_MS: cuanto se espera entre un paso y el siguiente.
// Con numeros mas chicos gira mas rapido (pero si baja demasiado, el
// motor pierde pasos o se traba); con numeros mas grandes gira mas lento
// pero mas firme. 2-4ms suele ser un buen punto de partida para el 28BYJ-48.
#define STEPPER_INTERVALO_MS 2
// =====================================================================

void Stepper_init(void);

// Define hacia donde tiene que girar para desplegar sombra (target > 0)
// o para regresar a reposo (target = 0). El movimiento real ocurre
// solo, poco a poco, cada vez que se llama Stepper_update().
void Stepper_set_objetivo(int32_t objetivo_pasos);

// Hay que llamar esta funcion en cada vuelta del loop principal.
// Se encarga de dar UN paso cuando ya toca (segun STEPPER_INTERVALO_MS),
// sin bloquear el resto del programa mientras tanto.
void Stepper_update(uint32_t ahora_ms);

uint8_t Stepper_esta_en_movimiento(void);
int32_t Stepper_posicion_actual(void);

#endif
/*
 * Relay.c
 *
 * Created: 30/07/2026 15:11:46
 *  Proyecto Invernadero - BE3029 Electronica Digital 2
 * Juan Daniel Sandoval 24209 y Fernando Guzman 24734
 */ 
#include "relay.h"

void Relay_init(void)
{
    // Se configura el pin como salida digital.
    RELAY_DDR |= (1 << RELAY_PIN);

    // Se arranca apagado (bomba detenida) por seguridad.
    RELAY_PORT &= ~(1 << RELAY_PIN);
}

void Relay_on(void)
{
    RELAY_PORT  &= ~ (1 << RELAY_PIN);
}

void Relay_off(void)
{
    RELAY_PORT |=(1 << RELAY_PIN);
}
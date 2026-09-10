/*
 * Control2_ATmega328P.c
 * Antirrebote ultra-reactivo e instantaneo (C puro / Microchip Studio)
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define BAUD 9600
#define MY_UBRR ((F_CPU / (16UL * BAUD)) - 1)

void UART_init(unsigned int ubrr);
void UART_transmit(char data);
void GPIO_init(void);

// Arreglos de pines y comandos correspondientes
const uint8_t pines[6]   = {PD2, PD3, PD4, PD5, PD6, PD7};
const char    comandos[6] = {'U', 'D', 'L', 'R', 'A', 'B'};

// Bandera para evitar que mande mil letras si dejas el boton presionado
uint8_t presionado[6] = {0, 0, 0, 0, 0, 0};

int main(void)
{
    UART_init(MY_UBRR);
    GPIO_init();

    while (1)
    {
        for (uint8_t i = 0; i < 6; i++)
        {
            // Si el pin esta en GND (boton presionado)
            if (!(PIND & (1 << pines[i])))
            {
                if (!presionado[i])
                {
                    _delay_ms(8); // Antirrebote rapido (8 ms)

                    // Reconfirmar que sigue presionado
                    if (!(PIND & (1 << pines[i])))
                    {
                        UART_transmit(comandos[i]); // Envio instantaneo
                        presionado[i] = 1;          // Bloquear repeticion hasta soltar
                    }
                }
            }
            else
            {
                // El pin volvio a 5V (boton liberado)
                presionado[i] = 0;
            }
        }

        _delay_ms(2); // Ciclo de muestreo veloz
    }
}

void UART_init(unsigned int ubrr)
{
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)(ubrr);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void GPIO_init(void)
{
    DDRD |= (1 << PD1); // TX salida
    // PD2 a PD7 como entradas con Pull-up habilitado
    DDRD  &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));
    PORTD |=  ((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));
}
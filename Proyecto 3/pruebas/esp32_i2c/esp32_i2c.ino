#include <Arduino.h>
#include <Wire.h>

constexpr int PIN_SDA = 21;
constexpr int PIN_SCL = 22;

constexpr uint8_t DIRECCION_STM1 = 0x10;
constexpr uint8_t DIRECCION_STM2 = 0x11;

constexpr uint32_t FRECUENCIA_I2C = 100000;
constexpr uint32_t PERIODO_CONSULTA_MS = 250;

bool busIniciado = false;

/*
 * Recibe exactamente un byte:
 * STM1: 0000 P1 P2 P3 P4
 * STM2: 0000 P5 P6 P7 P8
 *
 * Si falla la lectura, no inventar un estado 0000.
 */
bool leerEstadoLocal(uint8_t direccion, uint8_t &estado)
{
    size_t cantidad = Wire.requestFrom(
        direccion,
        static_cast<size_t>(1),
        true
    );

    if ((cantidad != 1U) || (Wire.available() < 1))
    {
        while (Wire.available() > 0)
        {
            Wire.read();
        }

        return false;
    }

    int recibido = Wire.read();

    if (recibido < 0)
    {
        return false;
    }

    uint8_t dato = static_cast<uint8_t>(recibido);

    /*
     * Los cuatro bits superiores deben ser cero
     * segun el protocolo acordado.
     */
    if ((dato & 0xF0U) != 0U)
    {
        Serial.printf(
            "Dato local invalido de 0x%02X: 0x%02X\r\n",
            static_cast<unsigned int>(direccion),
            static_cast<unsigned int>(dato)
        );

        return false;
    }

    estado = dato;
    return true;
}

/*
 * Devuelve el codigo de endTransmission:
 * 0 indica transferencia aceptada en el bus.
 */
uint8_t enviarEstadoGlobal(uint8_t direccion, uint8_t estado)
{
    Wire.beginTransmission(direccion);
    Wire.write(estado);
    return Wire.endTransmission(true);
}

void imprimirBits(uint8_t dato, uint8_t cantidad)
{
    for (int bit = static_cast<int>(cantidad) - 1; bit >= 0; bit--)
    {
        Serial.print((dato >> bit) & 1U);
    }
}

uint8_t contarOcupados(uint8_t estado)
{
    uint8_t cantidad = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        cantidad += (estado >> i) & 1U;
    }

    return cantidad;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32 - PRUEBA I2C DEL PARQUEO");
    Serial.println("STM1: direccion 0x10, parqueos P1-P4");
    Serial.println("STM2: direccion 0x11, parqueos P5-P8");
    Serial.println("SDA=GPIO21 | SCL=GPIO22 | 100 kHz");
    Serial.println("1=ocupado | 0=libre");
    Serial.println("========================================");

    busIniciado = Wire.begin(
        PIN_SDA,
        PIN_SCL,
        FRECUENCIA_I2C
    );

    if (!busIniciado)
    {
        Serial.println("ERROR: no se pudo iniciar I2C.");
        return;
    }

    Wire.setTimeOut(50);

    Serial.println(
        "Esperando placas con firmware I2C y calibracion terminada..."
    );
}

void loop()
{
    if (!busIniciado)
    {
        delay(1000);
        return;
    }

    uint8_t estadoSTM1 = 0;
    uint8_t estadoSTM2 = 0;

    /*
     * Consultar ambas placas, incluso si una falla.
     */
    bool lecturaSTM1 = leerEstadoLocal(
        DIRECCION_STM1,
        estadoSTM1
    );

    bool lecturaSTM2 = leerEstadoLocal(
        DIRECCION_STM2,
        estadoSTM2
    );

    if (!lecturaSTM1 || !lecturaSTM2)
    {
        Serial.printf(
            "Sin estado global actualizado | STM1: %s | STM2: %s\r\n",
            lecturaSTM1 ? "lectura OK" : "sin dato valido",
            lecturaSTM2 ? "lectura OK" : "sin dato valido"
        );

        Serial.println(
            "No se envia un nuevo estado global. "
            "Las placas deben detectar datos vencidos."
        );

        delay(PERIODO_CONSULTA_MS);
        return;
    }

    /*
     * Primer nibble: P1-P4.
     * Segundo nibble: P5-P8.
     */
    uint8_t estadoGlobal = static_cast<uint8_t>(
        (estadoSTM1 << 4) | estadoSTM2
    );

    uint8_t ocupados = contarOcupados(estadoGlobal);
    uint8_t libres = 8U - ocupados;

    uint8_t resultadoSTM1 = enviarEstadoGlobal(
        DIRECCION_STM1,
        estadoGlobal
    );

    uint8_t resultadoSTM2 = enviarEstadoGlobal(
        DIRECCION_STM2,
        estadoGlobal
    );

    Serial.print("STM1 [P1-P4]: ");
    imprimirBits(estadoSTM1, 4);

    Serial.print(" | STM2 [P5-P8]: ");
    imprimirBits(estadoSTM2, 4);

    Serial.println();

    Serial.print("GLOBAL [P1-P8]: ");
    imprimirBits(estadoGlobal, 8);

    Serial.printf(
        " | HEX: 0x%02X | Ocupados: %u | Libres: %u\r\n",
        static_cast<unsigned int>(estadoGlobal),
        static_cast<unsigned int>(ocupados),
        static_cast<unsigned int>(libres)
    );

    Serial.printf(
        "Envio global -> STM1: %s (codigo %u)"
        " | STM2: %s (codigo %u)\r\n",
        resultadoSTM1 == 0 ? "OK" : "ERROR",
        static_cast<unsigned int>(resultadoSTM1),
        resultadoSTM2 == 0 ? "OK" : "ERROR",
        static_cast<unsigned int>(resultadoSTM2)
    );

    Serial.println("----------------------------------------");

    delay(PERIODO_CONSULTA_MS);
}
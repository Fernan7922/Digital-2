/*
 * invernadero_adafruit.ino
 *
 * Recibe los datos del Nano Master por Serial2 (via el conversor de
 * nivel) y los sube a Adafruit IO en 3 feeds: T (temperatura), H
 * (humedad de suelo) y L (nivel de luz).
 *
 * El Nano sigue mandando el mismo texto de siempre por UART, no se le
 * toco nada; aqui solo se lee esa linea y se saca cada valor.
 *
 * Author: ferg7
 */


#include "AdafruitIO_WiFi.h"

#define WIFI_SSID "SOMOZA NETWORK"
#define WIFI_PASS "4D9697504328"

#define IO_USERNAME "Ferg7922"
#define IO_KEY "aio_cVkI79zl3uVLZ1Wfpwzx30sujTeG"

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
#define BAUD_MONITOR_USB 115200
#define BAUD_DESDE_NANO  9600

#define PIN_RX2 16
#define PIN_TX2 17

// Adafruit IO (con cuenta gratuita) deja 30 valores por minuto en
// total, y como aqui se mandan 3 de una vez, no conviene subir cada
// vez que llega una lectura del Nano (esas llegan cada 2s). Con 10s
// entre envios quedan unos 18 valores por minuto, con margen de sobra.
#define INTERVALO_ENVIO_MS 10000
unsigned long ultimo_envio = 0;

AdafruitIO_Feed *feed_temperatura = io.feed("Sensort");
AdafruitIO_Feed *feed_humedad     = io.feed("Sensorh");
AdafruitIO_Feed *feed_luz         = io.feed("Sensorl");

#define TAM_BUFFER_LINEA 128
char buffer_linea[TAM_BUFFER_LINEA];
uint8_t indice_buffer = 0;

float lectura_temp = 0;
int   lectura_hum  = 0;
int   lectura_luz  = 0;
bool  hay_lectura_nueva = false;

// Revisa si la linea que llego es la de reporte de sensores (no un
// aviso ni el mensaje de arranque) y saca los 3 valores de ahi.
bool parsearLectura(const String &linea)
{
	int pos_temp = linea.indexOf("Temp: ");
	int pos_hum  = linea.indexOf("Humedad suelo: ");
	int pos_luz  = linea.indexOf("LDR: ");

	if (pos_temp < 0 || pos_hum < 0 || pos_luz < 0)
	{
		return false;
	}

	String texto_temp = linea.substring(pos_temp + 6, pos_hum);
	String texto_hum  = linea.substring(pos_hum + 15, pos_luz);
	String texto_luz  = linea.substring(pos_luz + 5);

	texto_temp.trim();
	texto_hum.trim();
	texto_luz.trim();

	// Si alguno de los perifericos no contesto, el Nano manda ERROR o
	// SIN RESPUESTA en vez del numero. En ese caso mejor no publicar
	// nada en esa ronda, para no meter un dato falso a la grafica.
	if (texto_temp.indexOf("ERROR") >= 0) return false;
	if (texto_hum.indexOf("SIN RESPUESTA") >= 0) return false;
	if (texto_luz.indexOf("SIN RESPUESTA") >= 0) return false;

	lectura_temp = texto_temp.toFloat();
	lectura_hum  = texto_hum.toInt();
	lectura_luz  = texto_luz.toInt();

	return true;
}

void setup()
{
	Serial.begin(BAUD_MONITOR_USB);
	Serial2.begin(BAUD_DESDE_NANO, SERIAL_8N1, PIN_RX2, PIN_TX2);

	Serial.print("Conectando a Adafruit IO");
	io.connect();

	while (io.status() < AIO_CONNECTED)
	{
		Serial.print(".");
		delay(500);
	}

	Serial.println();
	Serial.println(io.statusText());
}

void loop()
{
	io.run();

	while (Serial2.available())
	{
		char c = Serial2.read();

		if (c == '\n')
		{
			buffer_linea[indice_buffer] = '\0';
			if (indice_buffer > 0)
			{
				String linea(buffer_linea);
				Serial.println(linea);

				if (parsearLectura(linea))
				{
					hay_lectura_nueva = true;
				}
			}
			indice_buffer = 0;
		}
		else if (c != '\r')
		{
			if (indice_buffer < (TAM_BUFFER_LINEA - 1))
			{
				buffer_linea[indice_buffer++] = c;
			}
		}
	}

	if (hay_lectura_nueva && millis() > (ultimo_envio + INTERVALO_ENVIO_MS))
	{
		Serial.print("Publicando -> T:");
		Serial.print(lectura_temp);
		Serial.print(" H:");
		Serial.print(lectura_hum);
		Serial.print(" L:");
		Serial.println(lectura_luz);

		feed_temperatura->save(lectura_temp);
		feed_humedad->save(lectura_hum);
		feed_luz->save(lectura_luz);

		ultimo_envio = millis();
		hay_lectura_nueva = false;
	}
}

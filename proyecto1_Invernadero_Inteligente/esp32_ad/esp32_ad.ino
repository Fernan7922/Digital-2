/*
 * 
 *
 * Recibe los datos del Nano Master por Serial2 (via el conversor de
 * nivel) y los sube a Adafruit IO en 3 feeds: T (temperatura), H
 * (humedad de suelo) y L (nivel de luz).
 *
 * Ademas, escucha 4 feeds de control (Modo, Bomba, Sombra,
 * Ventilacion) y cuando alguien toca un boton en el dashboard, le
 * manda el comando correspondiente al Nano por la misma linea UART,
 * pero en el sentido contrario (TX2 del ESP32 hacia el RX del Nano).
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

#define INTERVALO_ENVIO_MS 10000
unsigned long ultimo_envio = 0;

AdafruitIO_Feed *feed_temperatura = io.feed("Sensort");
AdafruitIO_Feed *feed_humedad     = io.feed("Sensorh");
AdafruitIO_Feed *feed_luz         = io.feed("Sensorl");

// Feeds de control: el toggle "Modo" decide si el invernadero trabaja
// solo (ON) o si se controla a mano con los otros 3 toggles (OFF).
AdafruitIO_Feed *feed_modo        = io.feed("Modo");
AdafruitIO_Feed *feed_bomba       = io.feed("Bomba");
AdafruitIO_Feed *feed_sombra      = io.feed("Sombra");
AdafruitIO_Feed *feed_ventilacion = io.feed("Ventilacion");

#define TAM_BUFFER_LINEA 128
char buffer_linea[TAM_BUFFER_LINEA];
uint8_t indice_buffer = 0;

float lectura_temp = 0;
int   lectura_hum  = 0;
int   lectura_luz  = 0;
bool  hay_lectura_nueva = false;

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

	if (texto_temp.indexOf("ERROR") >= 0) return false;
	if (texto_hum.indexOf("SIN RESPUESTA") >= 0) return false;
	if (texto_luz.indexOf("SIN RESPUESTA") >= 0) return false;

	lectura_temp = texto_temp.toFloat();
	lectura_hum  = texto_hum.toInt();
	lectura_luz  = texto_luz.toInt();

	return true;
}

// Adafruit IO manda el valor del toggle como texto, no como numero,
// asi que hay que comparar el texto directo (data->toBool() no sirve
// aqui: internamente hace atoi() sobre el texto, y eso da 0 tanto
// para "ON" como para cualquier otra palabra).
//
// Los toggles de Bomba, Sombra y Ventilacion mandan el texto por
// defecto ("ON"/"OFF"). El de Modo se dejo personalizado con los
// textos "Manual"/"Auto" en vez de "ON"/"OFF", asi que ese se compara
// aparte.
bool esOn(AdafruitIO_Data *data)
{
	return strcmp(data->value(), "ON") == 0;
}

// Estos 4 se llaman solos cuando alguien toca un boton en el
// dashboard de Adafruit IO. Lo unico que hacen es traducir eso a una
// linea de texto y mandarsela al Nano por el UART. El print al
// Monitor Serie es solo para confirmar a ojo que el toque del boton
// realmente esta saliendo hacia el Nano.
void alCambiarModo(AdafruitIO_Data *data)
{
	bool manual = (strcmp(data->value(), "Manual") == 0);
	Serial2.println(manual ? "MODO:MANUAL" : "MODO:AUTO");
	Serial.print("Adafruit IO -> ");
	Serial.println(manual ? "MODO:MANUAL" : "MODO:AUTO");
}

void alCambiarBomba(AdafruitIO_Data *data)
{
	bool encendida = esOn(data);
	Serial2.println(encendida ? "BOMBA:ON" : "BOMBA:OFF");
	Serial.print("Adafruit IO -> ");
	Serial.println(encendida ? "BOMBA:ON" : "BOMBA:OFF");
}

void alCambiarSombra(AdafruitIO_Data *data)
{
	bool desplegada = esOn(data);
	Serial2.println(desplegada ? "SOMBRA:ON" : "SOMBRA:OFF");
	Serial.print("Adafruit IO -> ");
	Serial.println(desplegada ? "SOMBRA:ON" : "SOMBRA:OFF");
}

void alCambiarVentilacion(AdafruitIO_Data *data)
{
	bool activa = esOn(data);
	Serial2.println(activa ? "VENT:ON" : "VENT:OFF");
	Serial.print("Adafruit IO -> ");
	Serial.println(activa ? "VENT:ON" : "VENT:OFF");
}

void setup()
{
	Serial.begin(BAUD_MONITOR_USB);
	Serial2.begin(BAUD_DESDE_NANO, SERIAL_8N1, PIN_RX2, PIN_TX2);

	feed_modo->onMessage(alCambiarModo);
	feed_bomba->onMessage(alCambiarBomba);
	feed_sombra->onMessage(alCambiarSombra);
	feed_ventilacion->onMessage(alCambiarVentilacion);

	Serial.print("Conectando a Adafruit IO");
	io.connect();

	while (io.status() < AIO_CONNECTED)
	{
		Serial.print(".");
		delay(500);
	}

	Serial.println();
	Serial.println(io.statusText());

	// Al reconectar, que se sincronice con el ultimo estado que haya
	// quedado guardado en cada feed (por si el ESP32 se reinicio y el
	// dashboard se quedo, por ejemplo, en manual).
	feed_modo->get();
	feed_bomba->get();
	feed_sombra->get();
	feed_ventilacion->get();
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

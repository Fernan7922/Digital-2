#define BAUD_MONITOR_USB   115200  // velocidad del Monitor Serie (USB, hacia tu PC)
#define BAUD_DESDE_NANO    9600    // coincide con UART_BAUD del Nano

void setup() {
  Serial.begin(BAUD_MONITOR_USB);
  Serial2.begin(BAUD_DESDE_NANO); // RX2=GPIO16, TX2=GPIO17 por defecto en ESP32

  Serial.println("Esperando datos del Nano Master...");
}

void loop() {
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial.write(c);
  }
}
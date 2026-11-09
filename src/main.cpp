// Guardia Clima · TP Integrador IoT (TA 4k01 · UTN FRC · 2026)
// Etapa 0: proyecto base. Solo arranca el monitor serie y muestra el mensaje de inicio.

#include <Arduino.h>
#include "config.h"

// Las credenciales viven en include/secrets.h, que no se sube al repo.
// Si falta, cortamos la compilacion con un mensaje claro en vez de un error confuso.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Falta include/secrets.h: copia include/secrets.example.h como secrets.h y completalo."
#endif

void setup() {
  Serial.begin(SERIAL_BAUDIOS);

  Serial.println();
  Serial.println("[SISTEMA] Guardia Clima iniciando...");
  Serial.printf("[SISTEMA] Core Arduino-ESP32 %d.%d.%d\n",
                ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
}

void loop() {
  // Vacio en la etapa 0: las tareas se agregan desde la etapa 1, sin delay().
}

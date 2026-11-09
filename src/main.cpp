// Guardia Clima · TP Integrador IoT (TA 4k01 · UTN FRC · 2026)
// El main solo coordina: cada responsabilidad vive en su libreria de lib/.

#include <Arduino.h>
#include "config.h"

// Las credenciales viven en include/secrets.h, que no se sube al repo.
// Si falta, cortamos la compilacion con un mensaje claro en vez de un error confuso.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Falta include/secrets.h: copia include/secrets.example.h como secrets.h y completalo."
#endif

#include "Actuador.h"
#include "Potenciometro.h"
#include "SensorAmbiente.h"
#include "Pantalla.h"

// --- Objetos del sistema (los constructores solo guardan parametros) ---
Actuador actuador(PIN_LED_VERDE);
Potenciometro potenciometro(PIN_POTENCIOMETRO, VOLTAJE_REFERENCIA, ADC_MAXIMO);
SensorAmbiente sensor(PIN_DHT, INTERVALO_DHT_MS);
Pantalla pantalla(OLED_ANCHO, OLED_ALTO, OLED_DIRECCION);

float ultimoVoltajeLogueado = -1.0f;  // -1 fuerza el primer log

void registrarLecturaDht(ResultadoLectura resultado);
void registrarPotenciometro();
void revisarComandoSerie();

void setup() {
  // 1. Serial
  Serial.begin(SERIAL_BAUDIOS);
  Serial.println();
  Serial.println("[SISTEMA] Guardia Clima iniciando...");

  // 2. Hardware local
  actuador.iniciar();
  pinMode(PIN_LED_AZUL, OUTPUT);
  digitalWrite(PIN_LED_AZUL, LOW);  // Se prende recien cuando haya WiFi (etapa 2)
  potenciometro.iniciar();
  sensor.iniciar();

  if (pantalla.iniciar()) {
    pantalla.mostrarInicio(EstadoWifi::CONECTANDO, "");
    Serial.println("[OLED] Display iniciado");
  } else {
    Serial.println("[OLED] Error: el display no responde en el bus I2C");
  }

  // 3. WiFi y 4. servicios de red: etapas 2 a 4.
  Serial.println("[SISTEMA] Listo. Prueba de pantallas: escribir led, pot, dht, rele u otro texto");
}

void loop() {
  registrarLecturaDht(sensor.actualizar());

  potenciometro.actualizar();
  registrarPotenciometro();

  revisarComandoSerie();
}

// Loguea solo cuando hubo un intento de lectura (cada 2 s), no en cada vuelta.
void registrarLecturaDht(ResultadoLectura resultado) {
  if (resultado == ResultadoLectura::NUEVA) {
    Serial.printf("[DHT] Temp: %.1f C | Hum: %.1f %%\n", sensor.temperatura(), sensor.humedad());
  } else if (resultado == ResultadoLectura::INVALIDA) {
    Serial.println("[DHT] Lectura invalida (NaN): se conserva la anterior");
  }
}

// El pote se lee en cada vuelta, pero solo se loguea si cambio de verdad:
// el ADC de la ESP32 tiene ruido y llenaria el monitor serie.
void registrarPotenciometro() {
  float voltaje = potenciometro.voltaje();
  if (fabsf(voltaje - ultimoVoltajeLogueado) >= VARIACION_LOG_VOLTAJE) {
    ultimoVoltajeLogueado = voltaje;
    Serial.printf("[POT] Lectura: %d | Voltaje: %.2f V\n", potenciometro.lectura(), voltaje);
  }
}

// Prueba de la etapa 1: elegir la pantalla del OLED desde el monitor serie,
// como el control por Serial de la Clase 2. En la etapa 6 lo reemplaza /display<cmd>.
void revisarComandoSerie() {
  if (Serial.available() == 0) {
    return;
  }
  String comando = Serial.readStringUntil('\n');
  comando.trim();
  comando.toLowerCase();

  float voltaje = potenciometro.voltaje();
  if (comando == "led") {
    pantalla.mostrarLeds(actuador.encendido(), digitalRead(PIN_LED_AZUL) == HIGH);
  } else if (comando == "pot") {
    pantalla.mostrarPotenciometro(potenciometro.lectura(), voltaje, UMBRAL_VOLTAJE,
                                  voltaje >= UMBRAL_VOLTAJE);
  } else if (comando == "dht") {
    pantalla.mostrarDht(sensor.hayLectura(), sensor.temperatura(), sensor.humedad());
  } else if (comando == "rele") {
    pantalla.mostrarRele(actuador.encendido(), 0);
  } else if (comando == "on") {
    actuador.encender();
  } else if (comando == "off") {
    actuador.apagar();
  } else if (comando == "inicio") {
    pantalla.mostrarInicio(EstadoWifi::CONECTANDO, "");
  } else {
    pantalla.mostrarNoIdentificado(comando);
  }
  Serial.println("[OLED] Pantalla: " + (comando.length() > 0 ? comando : String("(vacio)")));
}

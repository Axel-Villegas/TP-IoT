// Guardia Clima · TP Integrador IoT (TA 4k01 · UTN FRC · 2026)
// El main solo coordina: cada responsabilidad vive en su libreria de lib/.

#include <Arduino.h>
#include <WiFi.h>
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
#include "Nube.h"
#include "AlertaUmbral.h"
#include "BotTelegram.h"

// --- Estado del WiFi ---
// La escriben los eventos (otra tarea del sistema), por eso es volatile.
volatile bool wifiConectado = false;
// Ultimo estado que proceso el loop(): solo lo toca el loop(). Es tambien el
// estado del LED azul, por eso el bot lo recibe por referencia (para /displayled).
bool wifiAnterior = false;

// --- Objetos del sistema (los constructores solo guardan parametros) ---
Actuador actuador(PIN_LED_VERDE);
Potenciometro potenciometro(PIN_POTENCIOMETRO, VOLTAJE_REFERENCIA, ADC_MAXIMO);
SensorAmbiente sensor(PIN_DHT, INTERVALO_DHT_MS);
Pantalla pantalla(OLED_ANCHO, OLED_ALTO, OLED_DIRECCION);
Nube nube(TS_CHANNEL_ID, TS_WRITE_API_KEY, INTERVALO_THINGSPEAK_MS, INTERVALO_MINIMO_THINGSPEAK_MS);

AlertaUmbral alerta(UMBRAL_VOLTAJE);

const ConfiguracionBot configBot = {
  BOT_TOKEN, CHAT_ID, INTERVALO_BOT_MS, RELE_SEGUNDOS_DEFECTO, RELE_SEGUNDOS_MAXIMO
};
BotTelegram bot(configBot, {sensor, potenciometro, actuador, pantalla, nube, alerta, wifiAnterior});

// ThingSpeak usa HTTP (puerto 80): alcanza un WiFiClient sin TLS, como en la Practica 08.
WiFiClient clienteThingSpeak;

float ultimoVoltajeLogueado = -1.0f;  // -1 fuerza el primer log

void conectarWifi();
void alObtenerIp(WiFiEvent_t evento);
void alDesconectarWifi(WiFiEvent_t evento);
void revisarCambioWifi();
void aplicarEstadoWifi(bool conectado);
void registrarLecturaDht(ResultadoLectura resultado);
void registrarPotenciometro();

void setup() {
  // 1. Serial
  Serial.begin(SERIAL_BAUDIOS);
  Serial.println();
  Serial.println("[SISTEMA] Guardia Clima iniciando...");

  // 2. Hardware local
  actuador.iniciar();
  pinMode(PIN_LED_AZUL, OUTPUT);
  digitalWrite(PIN_LED_AZUL, LOW);  // Se prende recien cuando haya WiFi
  potenciometro.iniciar();
  sensor.iniciar();

  if (pantalla.iniciar()) {
    pantalla.mostrarInicio(EstadoWifi::CONECTANDO, "");
    Serial.println("[OLED] Display iniciado");
  } else {
    Serial.println("[OLED] Error: el display no responde en el bus I2C");
  }

  // 3. WiFi (con tiempo maximo: sin red, la placa sigue arrancando)
  conectarWifi();
  wifiAnterior = wifiConectado;
  aplicarEstadoWifi(wifiAnterior);  // Log, LED azul y pantalla con el estado inicial

  // 4. Servicios de red (no usan la red hasta que haya WiFi)
  nube.iniciar(clienteThingSpeak);
  bot.iniciar();
  Serial.printf("[SISTEMA] Heap libre al terminar el setup: %u bytes\r\n", ESP.getFreeHeap());

  Serial.println("[SISTEMA] Listo");
}

void loop() {
  registrarLecturaDht(sensor.actualizar());

  potenciometro.actualizar();
  registrarPotenciometro();

  revisarCambioWifi();

  // Antes que la red: un envio a ThingSpeak o una consulta a Telegram pueden
  // tardar, y el rele tiene que apagarse a tiempo igual.
  if (actuador.actualizar()) {
    bot.avisarFinRele();
  }

  // Vigilancia continua: en cada vuelta, no solo cuando alguien manda /thingup.
  // evaluar() devuelve true una sola vez por cruce del umbral.
  if (alerta.evaluar(potenciometro.voltaje())) {
    bot.enviarAlerta(potenciometro.voltaje());
  }

  nube.actualizar(sensor.hayLectura(), sensor.temperatura(), sensor.humedad(),
                  potenciometro.voltaje());

  bot.revisar();
}

// ---------------------------------------------------------------- WiFi

// Como en clase (Practicas 06 y 08): WiFi.begin y espera con puntos.
// Diferencia: espera como maximo TIMEOUT_WIFI_INICIAL_MS. Si no conecta,
// el driver sigue reintentando solo (autoReconnect) y los eventos avisan.
void conectarWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(alObtenerIp, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(alDesconectarWifi, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.printf("[WIFI] Conectando a %s", WIFI_SSID);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < TIMEOUT_WIFI_INICIAL_MS) {
    delay(500);  // Solo en el setup(), como en clase: el loop() todavia no arranco
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[WIFI] Sin conexion despues de %lu s: se sigue arrancando\r\n",
                  TIMEOUT_WIFI_INICIAL_MS / 1000);
  }
}

// Funciones de evento: corren en otra tarea del sistema, asi que son cortas
// y solo cambian la bandera. Nada de Serial ni LED aca: eso lo hace el loop().
void alObtenerIp(WiFiEvent_t evento) {
  wifiConectado = true;
}

void alDesconectarWifi(WiFiEvent_t evento) {
  wifiConectado = false;
}

// DISCONNECTED se repite en cada reintento del driver: comparando con el
// estado anterior, el log y el LED se aplican una sola vez por cambio.
void revisarCambioWifi() {
  bool actual = wifiConectado;  // Copia local: el evento puede cambiarla en cualquier momento
  if (actual != wifiAnterior) {
    wifiAnterior = actual;
    aplicarEstadoWifi(actual);
  }
}

void aplicarEstadoWifi(bool conectado) {
  digitalWrite(PIN_LED_AZUL, conectado ? HIGH : LOW);

  String ip = conectado ? WiFi.localIP().toString() : "";
  if (conectado) {
    Serial.println("[WIFI] Conectado, IP: " + ip);
  } else {
    Serial.println("[WIFI] Sin conexion, reintentando...");
  }

  // Solo si sigue en la pantalla de inicio: una pantalla de /display queda fija.
  if (pantalla.inicioVisible()) {
    pantalla.mostrarInicio(conectado ? EstadoWifi::CONECTADO : EstadoWifi::SIN_RED, ip);
  }
}

// ---------------------------------------------------------------- Logs de sensores

// Loguea solo cuando hubo un intento de lectura (cada 2 s), no en cada vuelta.
void registrarLecturaDht(ResultadoLectura resultado) {
  if (resultado == ResultadoLectura::NUEVA) {
    Serial.printf("[DHT] Temp: %.1f C | Hum: %.1f %%\r\n", sensor.temperatura(), sensor.humedad());
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
    Serial.printf("[POT] Lectura: %d | Voltaje: %.2f V\r\n", potenciometro.lectura(), voltaje);
  }
}

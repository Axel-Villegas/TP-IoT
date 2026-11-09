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
#include "BotTelegram.h"

// --- Objetos del sistema (los constructores solo guardan parametros) ---
Actuador actuador(PIN_LED_VERDE);
Potenciometro potenciometro(PIN_POTENCIOMETRO, VOLTAJE_REFERENCIA, ADC_MAXIMO);
SensorAmbiente sensor(PIN_DHT, INTERVALO_DHT_MS);
Pantalla pantalla(OLED_ANCHO, OLED_ALTO, OLED_DIRECCION);
Nube nube(TS_CHANNEL_ID, TS_WRITE_API_KEY, INTERVALO_THINGSPEAK_MS, INTERVALO_MINIMO_THINGSPEAK_MS);
BotTelegram bot(BOT_TOKEN, CHAT_ID, INTERVALO_BOT_MS, sensor, actuador,
                RELE_SEGUNDOS_DEFECTO, RELE_SEGUNDOS_MAXIMO);

// ThingSpeak usa HTTP (puerto 80): alcanza un WiFiClient sin TLS, como en la Practica 08.
WiFiClient clienteThingSpeak;

// --- Estado del WiFi ---
// La escriben los eventos (otra tarea del sistema), por eso es volatile.
volatile bool wifiConectado = false;
// Ultimo estado que proceso el loop(): solo lo toca el loop().
bool wifiAnterior = false;

// La pantalla de inicio se refresca con el WiFi solo mientras siga visible;
// una pantalla pedida con /display queda fija (spec, seccion 5).
bool pantallaInicioVisible = true;

float ultimoVoltajeLogueado = -1.0f;  // -1 fuerza el primer log

void conectarWifi();
void alObtenerIp(WiFiEvent_t evento);
void alDesconectarWifi(WiFiEvent_t evento);
void revisarCambioWifi();
void aplicarEstadoWifi(bool conectado);
void registrarLecturaDht(ResultadoLectura resultado);
void registrarPotenciometro();
void revisarComandoSerie();
void probarEnvioPedido();

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

  Serial.println("[SISTEMA] Listo. Prueba por serie: led, pot, dht, rele, inicio, on, off, ts");
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

  nube.actualizar(sensor.hayLectura(), sensor.temperatura(), sensor.humedad(),
                  potenciometro.voltaje());

  bot.revisar();

  revisarComandoSerie();
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

  if (pantallaInicioVisible) {
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

// ---------------------------------------------------------------- Prueba por serie

// Prueba de pantallas desde el monitor serie, como el control por Serial de la
// Clase 2. En la etapa 6 lo reemplaza /display<cmd>.
void revisarComandoSerie() {
  if (Serial.available() == 0) {
    return;
  }
  String comando = Serial.readStringUntil('\n');
  comando.trim();
  comando.toLowerCase();

  if (comando == "ts") {
    probarEnvioPedido();
    return;
  }

  float voltaje = potenciometro.voltaje();
  pantallaInicioVisible = false;
  if (comando == "led") {
    pantalla.mostrarLeds(actuador.encendido(), wifiAnterior);  // LED azul = estado del WiFi
  } else if (comando == "pot") {
    pantalla.mostrarPotenciometro(potenciometro.lectura(), voltaje, UMBRAL_VOLTAJE,
                                  voltaje >= UMBRAL_VOLTAJE);
  } else if (comando == "dht") {
    pantalla.mostrarDht(sensor.hayLectura(), sensor.temperatura(), sensor.humedad());
  } else if (comando == "rele") {
    pantalla.mostrarRele(actuador.encendido(), actuador.segundosRestantes());
  } else if (comando == "on") {
    actuador.encender(RELE_SEGUNDOS_DEFECTO);
  } else if (comando == "off") {
    actuador.apagar();
  } else if (comando == "inicio") {
    pantallaInicioVisible = true;
    pantalla.mostrarInicio(wifiAnterior ? EstadoWifi::CONECTADO : EstadoWifi::SIN_RED,
                           wifiAnterior ? WiFi.localIP().toString() : "");
  } else {
    pantalla.mostrarNoIdentificado(comando);
  }
  Serial.println("[OLED] Pantalla: " + (comando.length() > 0 ? comando : String("(vacio)")));
}

// Prueba de la etapa 3: simula el envio pedido de /thingup (etapa 7).
void probarEnvioPedido() {
  ResultadoEnvio r = nube.intentarEnvio(sensor.hayLectura(), sensor.temperatura(),
                                        sensor.humedad(), potenciometro.voltaje());
  switch (r) {
    case ResultadoEnvio::ENVIADO:
      Serial.printf("[TS] Prueba: enviado en el momento (codigo %d)\r\n", nube.ultimoResultado());
      break;
    case ResultadoEnvio::EN_ESPERA:
      Serial.printf("[TS] Prueba: queda pedido, sale en %lu s\r\n", nube.segundosParaProximoEnvio());
      break;
    case ResultadoEnvio::SIN_WIFI:
      Serial.println("[TS] Prueba: sin WiFi, sale cuando vuelva la red");
      break;
  }
}

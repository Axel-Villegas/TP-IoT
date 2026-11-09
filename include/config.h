#ifndef CONFIG_H
#define CONFIG_H

// Constantes del sistema: pines, umbral e intervalos.
// Se declaran constexpr para poder incluir este header desde varios archivos
// sin errores de definicion duplicada (cada constante queda interna a su archivo).

#include <stdint.h>

// --- Pines (enunciado, punto 1) ---
constexpr uint8_t PIN_DHT           = 33;  // DHT22 (digital, 1 hilo)
constexpr uint8_t PIN_POTENCIOMETRO = 32;  // ADC1: funciona con WiFi activo
constexpr uint8_t PIN_LED_VERDE     = 23;  // Representa al rele (actuador principal)
constexpr uint8_t PIN_LED_AZUL      = 2;   // Indicador de conexion WiFi

// --- Display OLED SH1106 (I2C: SDA 21, SCL 22) ---
constexpr uint8_t OLED_DIRECCION = 0x3C;
constexpr uint8_t OLED_ANCHO     = 128;
constexpr uint8_t OLED_ALTO      = 64;

// --- Potenciometro y alerta ---
constexpr float VOLTAJE_REFERENCIA = 3.3f;
constexpr int   ADC_MAXIMO         = 4095;  // 12 bits
constexpr float UMBRAL_VOLTAJE     = 2.8f;  // Mismo criterio que la Clase 2
constexpr float VARIACION_LOG_VOLTAJE = 0.05f;  // Cambio minimo para loguear (filtra el ruido del ADC)

// --- Intervalos (ms) ---
constexpr unsigned long INTERVALO_DHT_MS               = 2000;   // El DHT22 no lee mas rapido que cada 2 s
constexpr unsigned long INTERVALO_BOT_MS               = 1000;   // Polling de Telegram
constexpr unsigned long INTERVALO_THINGSPEAK_MS        = 20000;  // Envio periodico
constexpr unsigned long INTERVALO_MINIMO_THINGSPEAK_MS = 15000;  // Limite del canal gratuito
constexpr unsigned long TIMEOUT_WIFI_INICIAL_MS        = 20000;  // Espera maxima en el setup()

// --- Rele ---
constexpr unsigned long RELE_SEGUNDOS_DEFECTO = 5;
constexpr unsigned long RELE_SEGUNDOS_MAXIMO  = 3600;

// --- Monitor serie ---
constexpr unsigned long SERIAL_BAUDIOS = 115200;

#endif

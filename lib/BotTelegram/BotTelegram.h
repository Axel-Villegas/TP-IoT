#ifndef BOT_TELEGRAM_H
#define BOT_TELEGRAM_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "SensorAmbiente.h"
#include "Potenciometro.h"
#include "Actuador.h"
#include "Pantalla.h"

// Valores de configuracion del bot (salen de config.h y secrets.h en el main).
// Agruparlos en un struct evita un constructor con una lista larga de numeros
// sueltos, faciles de pasar en el orden equivocado.
struct ConfiguracionBot {
  const char* token;
  const char* chatAutorizado;
  unsigned long intervaloMs;
  unsigned long releSegundosDefecto;
  unsigned long releSegundosMaximo;
  float umbralVoltaje;
};

// Bot de Telegram (Practica 06): consulta mensajes nuevos cada intervaloMs
// (polling), atiende solo al chat autorizado y ejecuta cada comando.
// Los objetos que necesitan los comandos llegan por referencia en el
// constructor (inyeccion por constructor): el main no tiene logica de comandos.
class BotTelegram {
  public:
    BotTelegram(const ConfiguracionBot& config, SensorAmbiente& sensor,
                Potenciometro& potenciometro, Actuador& actuador, Pantalla& pantalla,
                const bool& ledAzulEncendido);
    void iniciar();
    void revisar();
    void avisarFinRele();

  private:
    WiFiClientSecure cliente;
    UniversalTelegramBot bot;
    ConfiguracionBot config;
    unsigned long ultimaRevision;
    bool primeraConsulta;
    SensorAmbiente& sensor;
    Potenciometro& potenciometro;
    Actuador& actuador;
    Pantalla& pantalla;
    const bool& ledAzulEncendido;

    void procesarMensajes(int cantidad);
    void ejecutarComando(const String& comando);
    void responder(const String& texto);
    void cmdClima();
    void cmdReleOn(const String& argumento);
    void cmdReleOff();
    bool leerSegundos(const String& argumento, unsigned long& segundos);
    void cmdDisplay(const String& sufijo);
};

#endif

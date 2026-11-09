#ifndef BOT_TELEGRAM_H
#define BOT_TELEGRAM_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "SensorAmbiente.h"
#include "Actuador.h"

// Bot de Telegram (Practica 06): consulta mensajes nuevos cada intervaloMs
// (polling), atiende solo al chat autorizado y ejecuta cada comando.
// Los objetos que necesitan los comandos llegan por referencia en el
// constructor (inyeccion por constructor): el main no tiene logica de comandos.
class BotTelegram {
  public:
    BotTelegram(const char* token, const char* chatAutorizado, unsigned long intervaloMs,
                SensorAmbiente& sensor, Actuador& actuador,
                unsigned long releSegundosDefecto, unsigned long releSegundosMaximo);
    void iniciar();
    void revisar();
    void avisarFinRele();

  private:
    WiFiClientSecure cliente;
    UniversalTelegramBot bot;
    String chatAutorizado;
    unsigned long intervaloMs;
    unsigned long ultimaRevision;
    bool primeraConsulta;
    SensorAmbiente& sensor;
    Actuador& actuador;
    unsigned long releSegundosDefecto;
    unsigned long releSegundosMaximo;

    void procesarMensajes(int cantidad);
    void ejecutarComando(const String& comando);
    void responder(const String& texto);
    void cmdClima();
    void cmdReleOn(const String& argumento);
    void cmdReleOff();
    bool leerSegundos(const String& argumento, unsigned long& segundos);
};

#endif

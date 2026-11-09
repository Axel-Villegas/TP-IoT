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
#include "Nube.h"
#include "AlertaUmbral.h"

// Valores de configuracion del bot (salen de config.h y secrets.h en el main).
// Agruparlos en un struct evita un constructor con una lista larga de numeros
// sueltos, faciles de pasar en el orden equivocado.
struct ConfiguracionBot {
  const char* token;
  const char* chatAutorizado;
  unsigned long intervaloMs;
  unsigned long releSegundosDefecto;
  unsigned long releSegundosMaximo;
};

// Objetos del sistema que usan los comandos (llegan por referencia).
struct ComponentesBot {
  SensorAmbiente& sensor;
  Potenciometro& potenciometro;
  Actuador& actuador;
  Pantalla& pantalla;
  Nube& nube;
  AlertaUmbral& alerta;
  const bool& ledAzulEncendido;
};

// Bot de Telegram (Practica 06): consulta mensajes nuevos cada intervaloMs
// (polling), atiende solo al chat autorizado y ejecuta cada comando.
// Los objetos que necesitan los comandos llegan por referencia en el
// constructor (inyeccion por constructor): el main no tiene logica de comandos.
class BotTelegram {
  public:
    BotTelegram(const ConfiguracionBot& config, const ComponentesBot& componentes);
    void iniciar();
    void revisar();
    void avisarFinRele();
    void enviarAlerta(float voltaje);

  private:
    WiFiClientSecure cliente;
    UniversalTelegramBot bot;
    ConfiguracionBot config;
    ComponentesBot componentes;
    unsigned long ultimaRevision;
    bool primeraConsulta;
    bool hayMensajesEnCola;

    // Alerta que no se pudo mandar en el momento del cruce (sin WiFi)
    bool alertaPendiente;
    float voltajeAlerta;
    unsigned long momentoAlerta;

    void procesarMensajes(int cantidad);
    void ejecutarComando(const String& comando);
    bool responder(const String& texto);
    bool responderConTeclado(const String& texto, const String& teclado);
    void confirmarBoton(const String& idBoton);
    void enviarAlertaPendiente();
    String textoUmbral();
    void cmdClima();
    void cmdReleOn(const String& argumento);
    void cmdReleOff();
    bool leerSegundos(const String& argumento, unsigned long& segundos);
    void cmdDisplay(const String& sufijo);
    void cmdThingup();
    void cmdStart();
    void cmdMenuDisplay();
};

#endif

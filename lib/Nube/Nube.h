#ifndef NUBE_H
#define NUBE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ThingSpeak.h>

// Que paso con un envio pedido por /thingup.
enum class ResultadoEnvio { ENVIADO, EN_ESPERA, SIN_WIFI };

// Publicacion en ThingSpeak (Practica 08): field1 = temperatura,
// field2 = humedad, field3 = voltaje del potenciometro.
// Envio periodico cada intervaloMs, o pedido (/thingup) apenas pasa
// intervaloMinimoMs desde el ultimo envio (limite de 15 s del canal).
class Nube {
  public:
    Nube(unsigned long canal, const char* apiKey,
         unsigned long intervaloMs, unsigned long intervaloMinimoMs);
    void iniciar(Client& cliente);
    void actualizar(bool hayDht, float temperatura, float humedad, float voltaje);
    void solicitarEnvio();
    ResultadoEnvio intentarEnvio(bool hayDht, float temperatura, float humedad, float voltaje);
    unsigned long segundosParaProximoEnvio();
    int ultimoResultado();

  private:
    unsigned long canal;
    const char* apiKey;
    unsigned long intervaloMs;
    unsigned long intervaloMinimoMs;
    unsigned long ultimoEnvio;
    bool envioSolicitado;
    int resultado;

    bool hayWifi();
    bool pasaron(unsigned long ms);
    void enviar(bool hayDht, float temperatura, float humedad, float voltaje, const char* motivo);
    const char* describirResultado(int codigo);
};

#endif

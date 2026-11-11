#ifndef ALERTA_UMBRAL_H
#define ALERTA_UMBRAL_H

#include <Arduino.h>

// Decide CUANDO avisar: una sola vez cada vez que el voltaje cruza el umbral
// de abajo hacia arriba. Mientras siga arriba no repite; al bajar queda lista
// para el proximo cruce. No sabe nada de Telegram: solo responde "hay que avisar".
class AlertaUmbral {
  public:
    AlertaUmbral(float umbral);
    bool evaluar(float voltaje);
    bool activa();
    float umbral();

  private:
    float umbralVoltaje;
    bool alertaEnviada;
};

#endif

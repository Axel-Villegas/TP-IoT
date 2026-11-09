#ifndef ACTUADOR_H
#define ACTUADOR_H

#include <Arduino.h>

// Actuador principal del sistema (el "rele"). En la placa UTN es el LED verde.
// Etapa 1: solo encender y apagar. El temporizador no bloqueante llega en la etapa 5.
class Actuador {
  public:
    Actuador(uint8_t pin);
    void iniciar();
    void encender();
    void apagar();
    bool encendido();

  private:
    uint8_t pin;
    bool estado;
};

#endif

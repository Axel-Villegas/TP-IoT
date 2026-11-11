#ifndef ACTUADOR_H
#define ACTUADOR_H

#include <Arduino.h>

// Actuador principal del sistema (el "rele"). En la placa UTN es el LED verde.
// Se enciende por X segundos y se apaga solo, sin bloquear: el tiempo se
// controla con millis() desde actualizar(), que se llama en cada vuelta del loop().
class Actuador {
  public:
    Actuador(uint8_t pin);
    void iniciar();
    void encender(unsigned long segundos);
    void apagar();
    bool actualizar();
    bool encendido();
    unsigned long segundosRestantes();

  private:
    uint8_t pin;
    bool estado;
    unsigned long momentoEncendido;
    unsigned long duracionMs;
};

#endif

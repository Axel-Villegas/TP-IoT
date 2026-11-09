#ifndef POTENCIOMETRO_H
#define POTENCIOMETRO_H

#include <Arduino.h>

// Lee el potenciometro por el ADC y lo convierte a voltaje (Clase 2).
class Potenciometro {
  public:
    Potenciometro(uint8_t pin, float vref, int adcMax);
    void iniciar();
    void actualizar();
    int lectura();
    float voltaje();

  private:
    uint8_t pin;
    float vref;
    int adcMax;
    int ultimaLectura;
};

#endif

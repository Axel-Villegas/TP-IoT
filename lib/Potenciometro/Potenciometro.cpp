#include "Potenciometro.h"

Potenciometro::Potenciometro(uint8_t pin, float vref, int adcMax)
    : pin(pin), vref(vref), adcMax(adcMax), ultimaLectura(0) {}

void Potenciometro::iniciar() {
  pinMode(pin, INPUT);
  analogReadResolution(12);  // 0 a 4095, como en la Clase 2
  actualizar();              // Asi voltaje() ya tiene un valor real desde el arranque
}

void Potenciometro::actualizar() {
  ultimaLectura = analogRead(pin);
}

int Potenciometro::lectura() {
  return ultimaLectura;
}

// Regla de tres: adcMax equivale a vref (4095 -> 3.3 V).
float Potenciometro::voltaje() {
  return ultimaLectura * vref / adcMax;
}

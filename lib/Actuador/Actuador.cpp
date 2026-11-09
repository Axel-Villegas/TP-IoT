#include "Actuador.h"

// El constructor solo guarda parametros: el hardware se configura en iniciar().
Actuador::Actuador(uint8_t pin) : pin(pin), estado(false) {}

void Actuador::iniciar() {
  pinMode(pin, OUTPUT);
  apagar();  // Arranca siempre apagado: nunca una carga encendida por accidente
}

void Actuador::encender() {
  digitalWrite(pin, HIGH);
  estado = true;
}

void Actuador::apagar() {
  digitalWrite(pin, LOW);
  estado = false;
}

bool Actuador::encendido() {
  return estado;
}

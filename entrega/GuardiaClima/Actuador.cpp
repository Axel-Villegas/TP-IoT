#include "Actuador.h"

// El constructor solo guarda parametros: el hardware se configura en iniciar().
Actuador::Actuador(uint8_t pin)
    : pin(pin), estado(false), momentoEncendido(0), duracionMs(0) {}

void Actuador::iniciar() {
  pinMode(pin, OUTPUT);
  apagar();  // Arranca siempre apagado: nunca una carga encendida por accidente
}

// Si ya estaba encendido, reinicia el temporizador con el tiempo nuevo.
void Actuador::encender(unsigned long segundos) {
  digitalWrite(pin, HIGH);
  estado = true;
  momentoEncendido = millis();
  duracionMs = segundos * 1000UL;
}

// Apaga y cancela el temporizador.
void Actuador::apagar() {
  digitalWrite(pin, LOW);
  estado = false;
  duracionMs = 0;
}

// Se llama en cada vuelta del loop(): apaga cuando se cumple el tiempo.
// Devuelve true solo en la vuelta en que se apago solo (para avisar una vez).
bool Actuador::actualizar() {
  if (estado && millis() - momentoEncendido >= duracionMs) {
    apagar();
    return true;
  }
  return false;
}

bool Actuador::encendido() {
  return estado;
}

// Redondea hacia arriba: con 0,4 s pendientes informa 1 s (nunca 0 s estando encendido).
unsigned long Actuador::segundosRestantes() {
  if (!estado) {
    return 0;
  }
  unsigned long transcurrido = millis() - momentoEncendido;
  if (transcurrido >= duracionMs) {
    return 0;
  }
  return (duracionMs - transcurrido + 999) / 1000;
}

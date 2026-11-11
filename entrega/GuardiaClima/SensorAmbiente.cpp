#include "SensorAmbiente.h"

SensorAmbiente::SensorAmbiente(uint8_t pin, unsigned long intervaloMs)
    : sensor(pin, DHT22),
      intervaloMs(intervaloMs),
      ultimoIntento(0),
      momentoLectura(0),
      lecturaValida(false),
      ultimaTemperatura(NAN),
      ultimaHumedad(NAN) {}

void SensorAmbiente::iniciar() {
  sensor.begin();
}

// No bloqueante: si todavia no paso el intervalo, vuelve enseguida.
ResultadoLectura SensorAmbiente::actualizar() {
  unsigned long ahora = millis();
  if (ahora - ultimoIntento < intervaloMs) {
    return ResultadoLectura::SIN_CAMBIOS;
  }
  ultimoIntento = ahora;

  float t = sensor.readTemperature();
  float h = sensor.readHumidity();

  // Una lectura fallida devuelve NaN: se descarta y queda la anterior.
  if (isnan(t) || isnan(h)) {
    return ResultadoLectura::INVALIDA;
  }

  ultimaTemperatura = t;
  ultimaHumedad = h;
  momentoLectura = ahora;
  lecturaValida = true;
  return ResultadoLectura::NUEVA;
}

bool SensorAmbiente::hayLectura() {
  return lecturaValida;
}

float SensorAmbiente::temperatura() {
  return ultimaTemperatura;
}

float SensorAmbiente::humedad() {
  return ultimaHumedad;
}

unsigned long SensorAmbiente::segundosDesdeLectura() {
  return (millis() - momentoLectura) / 1000;
}

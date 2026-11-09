#ifndef SENSOR_AMBIENTE_H
#define SENSOR_AMBIENTE_H

#include <Arduino.h>
#include <DHT.h>

// Resultado de cada llamada a actualizar(), para que el main sepa que loguear.
enum class ResultadoLectura { SIN_CAMBIOS, NUEVA, INVALIDA };

// DHT22: lee cada intervaloMs y guarda la ultima lectura valida.
// Basada en SensorDHT (Clase 3), pero sin tocar el hardware en el constructor.
class SensorAmbiente {
  public:
    SensorAmbiente(uint8_t pin, unsigned long intervaloMs);
    void iniciar();
    ResultadoLectura actualizar();
    bool hayLectura();
    float temperatura();
    float humedad();
    unsigned long segundosDesdeLectura();

  private:
    DHT sensor;
    unsigned long intervaloMs;
    unsigned long ultimoIntento;
    unsigned long momentoLectura;
    bool lecturaValida;
    float ultimaTemperatura;
    float ultimaHumedad;
};

#endif

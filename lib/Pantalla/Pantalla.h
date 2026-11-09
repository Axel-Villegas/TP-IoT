#ifndef PANTALLA_H
#define PANTALLA_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

enum class EstadoWifi { CONECTANDO, CONECTADO, SIN_RED };

// Todas las pantallas del OLED (spec.md, seccion 5).
// Fuente tamanio 1: 21 caracteres x 8 lineas, solo ASCII.
class Pantalla {
  public:
    Pantalla(uint8_t ancho, uint8_t alto, uint8_t direccion);
    bool iniciar();
    void mostrarInicio(EstadoWifi wifi, const String& ip);
    void mostrarLeds(bool verdeEncendido, bool azulEncendido);
    void mostrarPotenciometro(int lectura, float voltaje, float umbral, bool enAlerta);
    void mostrarDht(bool hayLectura, float temperatura, float humedad);
    void mostrarRele(bool encendido, unsigned long segundosRestantes);
    void mostrarNoIdentificado(const String& comando);

  private:
    static const uint8_t MAX_LINEAS = 8;
    static const uint8_t MAX_COLUMNAS = 21;

    Adafruit_SH1106G display;
    uint8_t direccion;

    void mostrarLineas(const String lineas[], uint8_t cantidad);
    String adaptarLinea(const String& texto);
};

#endif

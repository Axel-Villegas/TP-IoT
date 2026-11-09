#include "Pantalla.h"

static const char* SEPARADOR = "---------------------";  // 21 guiones = ancho completo

// El objeto del display solo guarda la configuracion; el bus I2C arranca en iniciar().
Pantalla::Pantalla(uint8_t ancho, uint8_t alto, uint8_t direccion)
    : display(ancho, alto, &Wire, -1), direccion(direccion), mostrandoInicio(false) {}

bool Pantalla::iniciar() {
  if (!display.begin(direccion, true)) {
    return false;  // No respondio en el bus I2C: el main lo informa por serie
  }
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();
  return true;
}

void Pantalla::mostrarInicio(EstadoWifi wifi, const String& ip) {
  String lineaWifi;
  switch (wifi) {
    case EstadoWifi::CONECTANDO: lineaWifi = "WiFi: conectando..."; break;
    case EstadoWifi::CONECTADO:  lineaWifi = "WiFi: conectado";     break;
    case EstadoWifi::SIN_RED:    lineaWifi = "WiFi: sin red";       break;
  }
  String lineaIp = ip.length() > 0 ? "IP: " + ip : "";

  String lineas[] = {"GUARDIA CLIMA", SEPARADOR, lineaWifi, lineaIp, "", "Esperando comandos"};
  mostrarLineas(lineas, 6);
  mostrandoInicio = true;  // Despues de mostrarLineas(), que lo pone en false
}

// La pantalla de inicio se refresca con el WiFi solo mientras siga visible;
// cualquier otra pantalla queda fija.
bool Pantalla::inicioVisible() {
  return mostrandoInicio;
}

void Pantalla::mostrarLeds(bool verdeEncendido, bool azulEncendido) {
  String lineas[] = {
    "ESTADO DE LOS LEDS",
    SEPARADOR,
    verdeEncendido ? "Led verde: encendido" : "Led verde: apagado",
    azulEncendido ? "Led azul: encendido" : "Led azul: apagado",
  };
  mostrarLineas(lineas, 4);
}

void Pantalla::mostrarPotenciometro(int lectura, float voltaje, float umbral, bool enAlerta) {
  String lineas[] = {
    "POTENCIOMETRO",
    SEPARADOR,
    "Lectura: " + String(lectura),
    "Voltaje: " + String(voltaje, 2) + " V",
    "Umbral:  " + String(umbral, 2) + " V",
    enAlerta ? "Estado: ALERTA" : "Estado: normal",
  };
  mostrarLineas(lineas, 6);
}

void Pantalla::mostrarDht(bool hayLectura, float temperatura, float humedad) {
  if (!hayLectura) {
    String lineas[] = {"SENSOR DHT22", SEPARADOR, "Sin lectura valida"};
    mostrarLineas(lineas, 3);
    return;
  }
  String lineas[] = {
    "SENSOR DHT22",
    SEPARADOR,
    "Temp: " + String(temperatura, 1) + " C",
    "Hum:  " + String(humedad, 1) + " %",
  };
  mostrarLineas(lineas, 4);
}

void Pantalla::mostrarRele(bool encendido, unsigned long segundosRestantes) {
  bool conTemporizador = encendido && segundosRestantes > 0;
  String lineas[] = {
    "RELE (LED VERDE)",
    SEPARADOR,
    encendido ? "Estado: ON" : "Estado: OFF",
    conTemporizador ? "Quedan: " + String(segundosRestantes) + " s" : "Sin temporizador",
  };
  mostrarLineas(lineas, 4);
}

void Pantalla::mostrarNoIdentificado(const String& comando) {
  String lineas[] = {
    "COMANDO NO",
    "IDENTIFICADO:",
    comando.length() > 0 ? comando : "(vacio)",
    "",
    "Validos: led, pot,",
    "dht, rele",
  };
  mostrarLineas(lineas, 6);
}

// Helper comun: limpia, escribe una linea por renglon y recien al final
// manda el buffer al display (un solo display() evita parpadeos).
void Pantalla::mostrarLineas(const String lineas[], uint8_t cantidad) {
  mostrandoInicio = false;  // Toda pantalla nueva reemplaza a la de inicio
  display.clearDisplay();
  display.setCursor(0, 0);
  for (uint8_t i = 0; i < cantidad && i < MAX_LINEAS; i++) {
    display.println(adaptarLinea(lineas[i]));
  }
  display.display();
}

// La fuente por defecto no tiene tildes ni simbolos UTF-8: cada caracter
// no ASCII se reemplaza por '?', y la linea se recorta a 21 caracteres
// para que no salte al renglon siguiente.
String Pantalla::adaptarLinea(const String& texto) {
  String resultado;
  for (unsigned int i = 0; i < texto.length() && resultado.length() < MAX_COLUMNAS; i++) {
    uint8_t c = texto[i];
    if (c >= 32 && c < 127) {
      resultado += (char)c;   // ASCII imprimible
    } else if (c >= 0xC0) {
      resultado += '?';       // Primer byte de un caracter UTF-8 (ej. 'a' con tilde)
    }
    // Bytes de continuacion UTF-8 (0x80-0xBF) y de control: se descartan
  }
  return resultado;
}

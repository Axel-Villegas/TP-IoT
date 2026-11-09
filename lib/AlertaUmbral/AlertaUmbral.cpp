#include "AlertaUmbral.h"

AlertaUmbral::AlertaUmbral(float umbral) : umbralVoltaje(umbral), alertaEnviada(false) {}

// Se llama en cada vuelta del loop(). Devuelve true solo en la vuelta del cruce.
// Es el patron "anterior vs. actual" del Practico 04: se actua cuando el estado
// CAMBIA, no mientras se mantiene (si no, saldrian cientos de alertas por segundo).
bool AlertaUmbral::evaluar(float voltaje) {
  if (voltaje >= umbralVoltaje && !alertaEnviada) {
    alertaEnviada = true;   // Recien cruzo: se avisa una vez
    return true;
  }
  if (voltaje < umbralVoltaje) {
    alertaEnviada = false;  // Bajo: queda lista para el proximo cruce
  }
  return false;
}

// true mientras el voltaje siga arriba del umbral (desde el ultimo evaluar()).
bool AlertaUmbral::activa() {
  return alertaEnviada;
}

float AlertaUmbral::umbral() {
  return umbralVoltaje;
}

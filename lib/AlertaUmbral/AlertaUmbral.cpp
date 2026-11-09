#include "AlertaUmbral.h"

AlertaUmbral::AlertaUmbral(float umbral) : umbralVoltaje(umbral), alertaEnviada(false) {}

// Se llama en cada vuelta del loop(). Devuelve true solo en la vuelta del
// cruce: se actua cuando el estado cambia, no mientras se mantiene.
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

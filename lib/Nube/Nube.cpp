#include "Nube.h"

Nube::Nube(unsigned long canal, const char* apiKey,
           unsigned long intervaloMs, unsigned long intervaloMinimoMs)
    : canal(canal),
      apiKey(apiKey),
      intervaloMs(intervaloMs),
      intervaloMinimoMs(intervaloMinimoMs),
      ultimoEnvio(0),
      huboEnvio(false),
      envioSolicitado(false),
      resultado(0) {}

// Recibe el cliente de red desde el main (un WiFiClient, sin TLS: ThingSpeak usa HTTP).
void Nube::iniciar(Client& cliente) {
  ThingSpeak.begin(cliente);
}

// Una sola regla para los dos tipos de envio:
// pedido pendiente y >= 15 s, o >= 20 s (periodico). Sin WiFi no se intenta.
void Nube::actualizar(bool hayDht, float temperatura, float humedad, float voltaje) {
  if (!hayWifi()) {
    return;
  }
  if (envioSolicitado && puedeEnviarPedido()) {
    // Si el ultimo envio fallo, esto es un reintento; si no, un /thingup en espera.
    enviar(hayDht, temperatura, humedad, voltaje, resultado != 200 && huboEnvio ? "reintento" : "pedido");
  } else if (pasaron(intervaloMs)) {
    enviar(hayDht, temperatura, humedad, voltaje, "periodico");
  }
}

void Nube::solicitarEnvio() {
  envioSolicitado = true;
}

// Lo usa /thingup: envia ya si se puede; si no, deja el envio pedido
// y actualizar() lo despacha apenas se cumplen los 15 s (o vuelve la red).
ResultadoEnvio Nube::intentarEnvio(bool hayDht, float temperatura, float humedad, float voltaje) {
  if (!hayWifi()) {
    solicitarEnvio();
    return ResultadoEnvio::SIN_WIFI;
  }
  if (puedeEnviarPedido()) {
    enviar(hayDht, temperatura, humedad, voltaje, "pedido");
    return ResultadoEnvio::ENVIADO;
  }
  solicitarEnvio();
  return ResultadoEnvio::EN_ESPERA;
}

// Con un pedido pendiente se cuenta hasta los 15 s; si no, hasta los 20 s.
// Redondea hacia arriba: si faltan 0,3 s, informa 1 s (nunca "0 s" antes de tiempo).
unsigned long Nube::segundosParaProximoEnvio() {
  if (envioSolicitado && puedeEnviarPedido()) {
    return 0;
  }
  unsigned long objetivo = envioSolicitado ? intervaloMinimoMs : intervaloMs;
  unsigned long transcurrido = millis() - ultimoEnvio;
  if (transcurrido >= objetivo) {
    return 0;
  }
  return (objetivo - transcurrido + 999) / 1000;
}

int Nube::ultimoResultado() {
  return resultado;
}

bool Nube::hayWifi() {
  return WiFi.status() == WL_CONNECTED;
}

bool Nube::pasaron(unsigned long ms) {
  return millis() - ultimoEnvio >= ms;
}

// El minimo de 15 s se cuenta desde el ultimo envio de ESTA ejecucion: si
// todavia no hubo ninguno (recien arranca la placa), un pedido sale ya.
// El periodico no usa esto: el primero sale a los 20 s, con el DHT ya leido.
bool Nube::puedeEnviarPedido() {
  return !huboEnvio || pasaron(intervaloMinimoMs);
}

// Unico lugar que habla con ThingSpeak: periodico y pedido comparten el codigo.
// Cada envio (salga bien o mal) reinicia el reloj, asi nunca hay dos a menos de 15 s.
// Si fallo (ej. -401 porque la placa se reinicio justo despues de un envio
// anterior, que ThingSpeak todavia cuenta), queda pedido: se reintenta a los
// 15 s en vez de esperar el periodico de 20 s, y el dato no se pierde.
void Nube::enviar(bool hayDht, float temperatura, float humedad, float voltaje, const char* motivo) {
  if (hayDht) {
    ThingSpeak.setField(1, temperatura);
    ThingSpeak.setField(2, humedad);
  }
  ThingSpeak.setField(3, voltaje);

  resultado = ThingSpeak.writeFields(canal, apiKey);
  ultimoEnvio = millis();
  huboEnvio = true;
  envioSolicitado = (resultado != 200);

  if (hayDht) {
    Serial.printf("[TS] Envio %s: T=%.1f H=%.1f V=%.2f -> %d (%s)\r\n",
                  motivo, temperatura, humedad, voltaje, resultado, describirResultado(resultado));
  } else {
    Serial.printf("[TS] Envio %s (sin DHT): V=%.2f -> %d (%s)\r\n",
                  motivo, voltaje, resultado, describirResultado(resultado));
  }
}

const char* Nube::describirResultado(int codigo) {
  switch (codigo) {
    case 200:  return "OK";
    case -401: return "rechazado: limite de 15 s o credenciales";
    case -301: return "no se pudo conectar";
    case -304: return "timeout del servidor";
    case 400:  return "API key incorrecta";
    default:   return "error";
  }
}

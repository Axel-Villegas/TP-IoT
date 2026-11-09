#include "Nube.h"

Nube::Nube(unsigned long canal, const char* apiKey,
           unsigned long intervaloMs, unsigned long intervaloMinimoMs)
    : canal(canal),
      apiKey(apiKey),
      intervaloMs(intervaloMs),
      intervaloMinimoMs(intervaloMinimoMs),
      ultimoEnvio(0),
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
  if (envioSolicitado && pasaron(intervaloMinimoMs)) {
    enviar(hayDht, temperatura, humedad, voltaje, "pedido");
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
  if (pasaron(intervaloMinimoMs)) {
    enviar(hayDht, temperatura, humedad, voltaje, "pedido");
    return ResultadoEnvio::ENVIADO;
  }
  solicitarEnvio();
  return ResultadoEnvio::EN_ESPERA;
}

// Con un pedido pendiente se cuenta hasta los 15 s; si no, hasta los 20 s.
// Redondea hacia arriba: si faltan 0,3 s, informa 1 s (nunca "0 s" antes de tiempo).
unsigned long Nube::segundosParaProximoEnvio() {
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

// Unico lugar que habla con ThingSpeak: periodico y pedido comparten el codigo.
// Cada envio (salga bien o mal) reinicia el reloj, asi nunca hay dos a menos de 15 s.
void Nube::enviar(bool hayDht, float temperatura, float humedad, float voltaje, const char* motivo) {
  if (hayDht) {
    ThingSpeak.setField(1, temperatura);
    ThingSpeak.setField(2, humedad);
  }
  ThingSpeak.setField(3, voltaje);

  resultado = ThingSpeak.writeFields(canal, apiKey);
  ultimoEnvio = millis();
  envioSolicitado = false;

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

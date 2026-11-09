#include "BotTelegram.h"

// El bot recibe el cliente seguro por referencia: se construye antes que el bot
// porque esta declarado primero en la clase (los miembros se inicializan en ese orden).
BotTelegram::BotTelegram(const char* token, const char* chatAutorizado,
                         unsigned long intervaloMs, SensorAmbiente& sensor)
    : cliente(),
      bot(token, cliente),
      chatAutorizado(chatAutorizado),
      intervaloMs(intervaloMs),
      ultimaRevision(0),
      primeraConsulta(true),
      sensor(sensor) {}

// Telegram solo acepta HTTPS: el certificado raiz permite verificar
// que del otro lado esta realmente api.telegram.org (como en la Practica 06).
void BotTelegram::iniciar() {
  cliente.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

// Polling no bloqueante: cada intervaloMs pregunta si hay mensajes nuevos.
// Sin WiFi no lo intenta: getUpdates() trabaria el loop() esperando un timeout.
void BotTelegram::revisar() {
  if (millis() - ultimaRevision < intervaloMs) {
    return;
  }
  ultimaRevision = millis();

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // last_message_received + 1 le dice a Telegram "ya procese hasta este":
  // asi cada mensaje llega una sola vez.
  int cantidad = bot.getUpdates(bot.last_message_received + 1);

  if (primeraConsulta) {
    primeraConsulta = false;
    Serial.printf("[BOT] Primera consulta a Telegram hecha. Heap libre: %u bytes\n",
                  ESP.getFreeHeap());
  }

  while (cantidad > 0) {
    procesarMensajes(cantidad);
    cantidad = bot.getUpdates(bot.last_message_received + 1);
  }
}

void BotTelegram::procesarMensajes(int cantidad) {
  for (int i = 0; i < cantidad; i++) {
    String chatId = bot.messages[i].chat_id;
    String texto = bot.messages[i].text;

    // Seguridad basica: solo el chat de secrets.h. El resto se ignora,
    // pero se loguea su chat_id (sirve para averiguar el propio).
    if (chatId != chatAutorizado) {
      Serial.println("[BOT] Chat no autorizado: " + chatId + " (texto: " + texto + ")");
      continue;
    }

    // Normalizacion: "  /CLIMA " y "/clima" son el mismo comando.
    texto.trim();
    texto.toLowerCase();
    Serial.println("[BOT] Comando recibido: " + texto);
    ejecutarComando(texto);
  }
}

// Despacho de comandos: un metodo por comando. Se completa en las etapas 5 a 8.
void BotTelegram::ejecutarComando(const String& comando) {
  if (comando == "/clima") {
    cmdClima();
  } else {
    responder("❓ No reconozco ese comando. Mandá /start para ver las opciones.");
  }
}

// Todas las respuestas van al chat autorizado (es el unico que se atiende).
void BotTelegram::responder(const String& texto) {
  if (!bot.sendMessage(chatAutorizado, texto)) {
    Serial.println("[BOT] Error al enviar la respuesta");
  }
}

// "Ultimos valores registrados": se responde con la lectura guardada,
// no se fuerza una lectura nueva (el DHT22 necesita 2 s entre lecturas).
void BotTelegram::cmdClima() {
  if (!sensor.hayLectura()) {
    responder("⚠️ Todavía no hay una lectura válida del DHT22.");
    return;
  }
  String texto = "🌡 Temperatura: " + String(sensor.temperatura(), 1) + " °C\n";
  texto += "💧 Humedad: " + String(sensor.humedad(), 1) + " %\n";
  texto += "🕒 Lectura de hace " + String(sensor.segundosDesdeLectura()) + " s";
  responder(texto);
}

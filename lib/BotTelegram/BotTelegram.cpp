#include "BotTelegram.h"

// El bot recibe el cliente seguro por referencia: se construye antes que el bot
// porque esta declarado primero en la clase (los miembros se inicializan en ese orden).
BotTelegram::BotTelegram(const ConfiguracionBot& config, SensorAmbiente& sensor,
                         Potenciometro& potenciometro, Actuador& actuador, Pantalla& pantalla,
                         const bool& ledAzulEncendido)
    : cliente(),
      bot(config.token, cliente),
      config(config),
      ultimaRevision(0),
      primeraConsulta(true),
      sensor(sensor),
      potenciometro(potenciometro),
      actuador(actuador),
      pantalla(pantalla),
      ledAzulEncendido(ledAzulEncendido) {}

// Telegram solo acepta HTTPS: el certificado raiz permite verificar
// que del otro lado esta realmente api.telegram.org (como en la Practica 06).
void BotTelegram::iniciar() {
  cliente.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

// Polling no bloqueante: cada intervaloMs pregunta si hay mensajes nuevos.
// Sin WiFi no lo intenta: getUpdates() trabaria el loop() esperando un timeout.
void BotTelegram::revisar() {
  if (millis() - ultimaRevision < config.intervaloMs) {
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
    Serial.printf("[BOT] Primera consulta a Telegram hecha. Heap libre: %u bytes\r\n",
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
    if (chatId != config.chatAutorizado) {
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

// Despacho de comandos: un metodo por comando. Se completa en las etapas 7 y 8.
void BotTelegram::ejecutarComando(const String& comando) {
  if (comando == "/clima") {
    cmdClima();
  } else if (comando == "/releoff" || comando == "/rele off") {
    cmdReleOff();
  } else if (comando.startsWith("/rele on")) {
    cmdReleOn(comando.substring(8));  // Alias con espacio: "/rele on 10"
  } else if (comando.startsWith("/releon")) {
    cmdReleOn(comando.substring(7));  // "/releon 10" -> " 10"
  } else if (comando.startsWith("/display")) {
    cmdDisplay(comando.substring(8));  // "/displayled" -> "led"
  } else {
    responder("❓ No reconozco ese comando. Mandá /start para ver las opciones.");
  }
}

// Todas las respuestas van al chat autorizado (es el unico que se atiende).
void BotTelegram::responder(const String& texto) {
  if (!bot.sendMessage(config.chatAutorizado, texto)) {
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

// /releon [seg]: sin numero usa el valor por defecto. Si ya estaba encendido,
// encender() reinicia el temporizador con el tiempo nuevo.
void BotTelegram::cmdReleOn(const String& argumento) {
  unsigned long segundos;
  if (!leerSegundos(argumento, segundos)) {
    responder("❌ Tiempo inválido. Usá /releon <segundos> (de 1 a " +
              String(config.releSegundosMaximo) + ").");
    return;
  }
  actuador.encender(segundos);
  Serial.printf("[RELE] Encendido por %lu s\r\n", segundos);
  responder("✅ Relé encendido por " + String(segundos) + " s. Se apaga solo.");
}

void BotTelegram::cmdReleOff() {
  if (!actuador.encendido()) {
    responder("ℹ️ El relé ya estaba apagado.");
    return;
  }
  actuador.apagar();
  Serial.println("[RELE] Apagado por comando");
  responder("⏹ Relé apagado.");
}

// Valida el argumento de /releon: vacio -> por defecto; si no, solo digitos
// y entre 1 y el maximo. Se chequea el largo antes de convertir para que un
// numero gigante no desborde (ej. "99999999999").
bool BotTelegram::leerSegundos(const String& argumento, unsigned long& segundos) {
  String texto = argumento;
  texto.trim();
  if (texto.length() == 0) {
    segundos = config.releSegundosDefecto;
    return true;
  }
  if (texto.length() > 5) {
    return false;
  }
  for (unsigned int i = 0; i < texto.length(); i++) {
    if (!isDigit(texto[i])) {
      return false;  // "abc", "-5", "1.5" -> invalido
    }
  }
  segundos = texto.toInt();
  return segundos >= 1 && segundos <= config.releSegundosMaximo;
}

// /display<cmd>: el sufijo decide que pantalla mostrar. Es una "foto" del
// momento del comando: queda fija hasta el proximo /display (spec, seccion 5).
// Cualquier otro sufijo (incluido vacio) se informa en el OLED y por Telegram.
void BotTelegram::cmdDisplay(const String& sufijo) {
  String cmd = sufijo;
  cmd.trim();  // "/display led" se acepta igual que "/displayled"

  String componente;
  if (cmd == "led") {
    pantalla.mostrarLeds(actuador.encendido(), ledAzulEncendido);
    componente = "LEDs";
  } else if (cmd == "pot") {
    float voltaje = potenciometro.voltaje();
    pantalla.mostrarPotenciometro(potenciometro.lectura(), voltaje, config.umbralVoltaje,
                                  voltaje >= config.umbralVoltaje);
    componente = "potenciómetro";
  } else if (cmd == "dht") {
    pantalla.mostrarDht(sensor.hayLectura(), sensor.temperatura(), sensor.humedad());
    componente = "DHT22";
  } else if (cmd == "rele") {
    pantalla.mostrarRele(actuador.encendido(), actuador.segundosRestantes());
    componente = "relé";
  } else {
    pantalla.mostrarNoIdentificado(cmd);
    String mostrado = cmd.length() > 0 ? cmd : "(vacío)";
    Serial.println("[OLED] Comando de display no identificado: " + mostrado);
    responder("❓ Comando de display no identificado: " + mostrado +
              ". Se informó en la pantalla.");
    return;
  }

  Serial.println("[OLED] Pantalla: " + cmd);
  responder("🖥 Mostrando " + componente + " en el display.");
}

// Lo llama el main cuando el temporizador del actuador termina.
void BotTelegram::avisarFinRele() {
  Serial.println("[RELE] Tiempo cumplido: apagado");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[RELE] Sin WiFi: el aviso por Telegram no se envia");
    return;
  }
  responder("⏱ Se cumplió el tiempo: relé apagado.");
}

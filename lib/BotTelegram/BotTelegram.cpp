#include "BotTelegram.h"

// El bot recibe el cliente seguro por referencia: se construye antes que el bot
// porque esta declarado primero en la clase (los miembros se inicializan en ese orden).
BotTelegram::BotTelegram(const ConfiguracionBot& config, const ComponentesBot& componentes)
    : cliente(),
      bot(config.token, cliente),
      config(config),
      componentes(componentes),
      ultimaRevision(0),
      primeraConsulta(true),
      alertaPendiente(false),
      voltajeAlerta(0),
      momentoAlerta(0) {}

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

  // Primero lo que quedo pendiente de cuando no habia red.
  if (alertaPendiente) {
    enviarAlertaPendiente();
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

// Despacho de comandos: un metodo por comando. /start llega en la etapa 8.
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
  } else if (comando == "/thingup") {
    cmdThingup();
  } else {
    responder("❓ No reconozco ese comando. Mandá /start para ver las opciones.");
  }
}

// Todas las respuestas van al chat autorizado (es el unico que se atiende).
// Devuelve si Telegram confirmo el envio (lo usa la alerta para no perderse).
bool BotTelegram::responder(const String& texto) {
  if (!bot.sendMessage(config.chatAutorizado, texto)) {
    Serial.println("[BOT] Error al enviar la respuesta");
    return false;
  }
  return true;
}

// "2.80": el umbral con 2 decimales, para todos los mensajes.
String BotTelegram::textoUmbral() {
  return String(componentes.alerta.umbral(), 2);
}

// ---------------------------------------------------------------- Alerta automatica

// La llama el main en el momento del cruce, sin que nadie la pida.
// Sin WiFi (o si Telegram no la acepta) queda pendiente con el voltaje y el
// momento del cruce, para mandarla apenas vuelva la conexion.
void BotTelegram::enviarAlerta(float voltaje) {
  Serial.printf("[ALERTA] Cruzo el umbral: %.2f V\r\n", voltaje);

  if (WiFi.status() == WL_CONNECTED &&
      responder("🚨 ALERTA: el potenciómetro marca " + String(voltaje, 2) +
                " V y supera el umbral crítico de " + textoUmbral() + " V.")) {
    Serial.println("[ALERTA] Enviada");
    return;
  }

  // Si ya habia una pendiente, se conserva la primera: es el cruce mas viejo sin avisar.
  if (!alertaPendiente) {
    alertaPendiente = true;
    voltajeAlerta = voltaje;
    momentoAlerta = millis();
  }
  Serial.println("[ALERTA] Sin conexion: queda pendiente");
}

// Queda registro del cruce aunque el valor ya se haya normalizado; si es asi,
// se agrega un segundo mensaje con el valor actual (spec RF6).
void BotTelegram::enviarAlertaPendiente() {
  unsigned long segundos = (millis() - momentoAlerta) / 1000;
  bool enviada = responder("🚨 ALERTA (registrada sin conexión): hace " + String(segundos) +
                           " s el potenciómetro llegó a " + String(voltajeAlerta, 2) +
                           " V y superó el umbral crítico de " + textoUmbral() + " V.");
  if (!enviada) {
    return;  // Sigue pendiente: se reintenta en la proxima revision
  }
  alertaPendiente = false;
  Serial.println("[ALERTA] Pendiente enviada");

  float voltajeActual = componentes.potenciometro.voltaje();
  if (voltajeActual < componentes.alerta.umbral()) {
    responder("✅ Ahora el valor es normal: " + String(voltajeActual, 2) + " V.");
  }
}

// ---------------------------------------------------------------- Comandos

// "Ultimos valores registrados": se responde con la lectura guardada,
// no se fuerza una lectura nueva (el DHT22 necesita 2 s entre lecturas).
void BotTelegram::cmdClima() {
  SensorAmbiente& sensor = componentes.sensor;
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
  componentes.actuador.encender(segundos);
  Serial.printf("[RELE] Encendido por %lu s\r\n", segundos);
  responder("✅ Relé encendido por " + String(segundos) + " s. Se apaga solo.");
}

void BotTelegram::cmdReleOff() {
  if (!componentes.actuador.encendido()) {
    responder("ℹ️ El relé ya estaba apagado.");
    return;
  }
  componentes.actuador.apagar();
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

  Pantalla& pantalla = componentes.pantalla;
  String componente;
  if (cmd == "led") {
    pantalla.mostrarLeds(componentes.actuador.encendido(), componentes.ledAzulEncendido);
    componente = "LEDs";
  } else if (cmd == "pot") {
    pantalla.mostrarPotenciometro(componentes.potenciometro.lectura(),
                                  componentes.potenciometro.voltaje(),
                                  componentes.alerta.umbral(), componentes.alerta.activa());
    componente = "potenciómetro";
  } else if (cmd == "dht") {
    SensorAmbiente& sensor = componentes.sensor;
    pantalla.mostrarDht(sensor.hayLectura(), sensor.temperatura(), sensor.humedad());
    componente = "DHT22";
  } else if (cmd == "rele") {
    pantalla.mostrarRele(componentes.actuador.encendido(), componentes.actuador.segundosRestantes());
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

// /thingup: envia a ThingSpeak en el momento si se puede (>= 15 s desde el
// ultimo envio); si no, queda pedido. Informa el voltaje, que paso con el
// envio y si supera el umbral.
void BotTelegram::cmdThingup() {
  Potenciometro& pot = componentes.potenciometro;
  SensorAmbiente& sensor = componentes.sensor;
  Nube& nube = componentes.nube;
  float voltaje = pot.voltaje();

  String texto = "🔌 Potenciómetro: " + String(voltaje, 2) + " V (lectura " +
                 String(pot.lectura()) + ")\n";

  ResultadoEnvio resultado = nube.intentarEnvio(sensor.hayLectura(), sensor.temperatura(),
                                                sensor.humedad(), voltaje);
  switch (resultado) {
    case ResultadoEnvio::ENVIADO:
      texto += "☁️ Enviado a ThingSpeak (field3).";
      if (nube.ultimoResultado() != 200) {
        texto += "\n⚠️ ThingSpeak respondió " + String(nube.ultimoResultado()) + ".";
      }
      break;
    case ResultadoEnvio::EN_ESPERA:
      texto += "☁️ Se envía a ThingSpeak en " + String(nube.segundosParaProximoEnvio()) +
               " s (mínimo 15 s entre envíos).";
      break;
    case ResultadoEnvio::SIN_WIFI:
      texto += "☁️ Sin conexión: se envía cuando vuelva la red.";
      break;
  }

  if (componentes.alerta.activa()) {
    texto += "\n⚠️ Supera el umbral crítico de " + textoUmbral() + " V.";
  }
  responder(texto);
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

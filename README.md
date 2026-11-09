# Guardia Clima · TP Integrador IoT

Sistema embebido basado en **ESP32** para la **supervisión ambiental** y la **actuación remota**, integrado con la plataforma IoT **ThingSpeak** y un **bot de Telegram**.

Trabajo Práctico Integrador de **Tecnologías para la Automatización (TA) · 4k01 · 2026**, Universidad Tecnológica Nacional, Facultad Regional Córdoba.

---

## Qué hace

- Mide **temperatura y humedad** con el sensor DHT22 y el **voltaje** de un potenciómetro.
- Publica las lecturas en un canal de **ThingSpeak** para verlas graficadas en tiempo real.
- Se controla desde **Telegram**: consultar el clima, encender un relé por un tiempo y elegir qué se muestra en el display.
- **Vigila el potenciómetro todo el tiempo** y, si supera un umbral crítico, avisa por Telegram sin que nadie lo pida.
- Muestra estados en el **display OLED** de la placa.

## Hardware

Placa de desarrollo UTN (*Evaluation Board V1.0*) con ESP32 DevKit V1 de 30 pines.

| Componente | Pin | Uso en el sistema |
| --- | --- | --- |
| Sensor DHT22 | GPIO33 | Temperatura y humedad relativa |
| Potenciómetro | GPIO32 (ADC1) | Señal analógica "crítica" (0 a 3,3 V) |
| LED verde | GPIO23 | Actuador principal (representa al relé) |
| LED azul | GPIO2 | Indicador de conexión WiFi |
| Display OLED SH1106 128×64 | SDA GPIO21 · SCL GPIO22 (I²C, 0x3C) | Estado de los componentes |

> ⚠️ Regla de la placa: **no alimentar la ESP32 desde DC y USB a la vez**.

En la simulación de Wokwi se agrega además un módulo relé en paralelo con el LED verde (mismo GPIO23), para ver el actuador real.

## Comandos del bot

| Comando | Qué hace |
| --- | --- |
| `/start` | Muestra el menú con botones (Ver Clima, Encender Relé, Ver Display) |
| `/clima` | Devuelve la última lectura de temperatura y humedad |
| `/releon <seg>` | Enciende el relé durante `<seg>` segundos y lo apaga solo (por defecto 5 s) |
| `/releoff` | Apaga el relé y cancela el temporizador |
| `/thingup` | Informa el voltaje del potenciómetro, que se publica en ThingSpeak, e indica si supera el umbral |
| `/displayled` | Muestra en el OLED el estado de los LEDs |
| `/displaypot` | Muestra en el OLED la lectura y el voltaje del potenciómetro |
| `/displaydht` | Muestra en el OLED la temperatura y la humedad |
| `/displayrele` | Muestra en el OLED el estado del relé y los segundos restantes |

Cualquier otro `/display...` se informa en el OLED como **comando no identificado**.

**Alerta automática:** cuando el voltaje del potenciómetro cruza los **2,8 V**, el bot avisa **una sola vez**. Vuelve a avisar recién después de que el voltaje baje del umbral y lo cruce de nuevo.

## ThingSpeak

| Campo | Dato |
| --- | --- |
| Field 1 | Temperatura (°C) |
| Field 2 | Humedad (%) |
| Field 3 | Voltaje del potenciómetro (V) |

Se publica cada 20 s.

## Arquitectura

El programa principal (`src/main.cpp`) solo coordina. Cada parte del sistema es una **librería propia** en `lib/`:

| Librería | Responsabilidad |
| --- | --- |
| `SensorAmbiente` | Lee el DHT22 cada 2 s y guarda la última lectura válida |
| `Potenciometro` | Lee el ADC y lo convierte a voltaje |
| `AlertaUmbral` | Decide cuándo avisar: una sola vez al cruzar el umbral |
| `Actuador` | Enciende el relé X segundos y lo apaga solo, sin bloquear |
| `Pantalla` | Todos los mensajes del display OLED |
| `Nube` | Publicación en ThingSpeak respetando el límite de tiempo |
| `BotTelegram` | Recibe e interpreta comandos, teclado inline y alertas |

Ninguna tarea bloquea a las demás: cada librería lleva su propio reloj con `millis()` y el `loop()` nunca usa `delay()`.

```
TP-IoT/
├─ platformio.ini
├─ diagram.json          ← circuito de Wokwi
├─ wokwi.toml
├─ include/
│  ├─ config.h            ← pines, umbral, intervalos
│  └─ secrets.example.h   ← plantilla de credenciales
├─ lib/                   ← librerías propias
└─ src/
   └─ main.cpp
```

## Cómo usarlo

### Requisitos

- Visual Studio Code con la extensión **PlatformIO IDE**.
- Extensión **Wokwi Simulator** con licencia activa (para simular).
- Una cuenta de **ThingSpeak** con un canal de 3 campos y un **bot de Telegram** creado con BotFather.

### Configurar las credenciales

1. Copiar `include/secrets.example.h` como `include/secrets.h`.
2. Completar la red WiFi, el token del bot, el chat autorizado y la Write API Key del canal.
3. `secrets.h` está en el `.gitignore`: **nunca se sube al repositorio**.

Para simular en Wokwi, la red es `Wokwi-GUEST` sin contraseña.

### Compilar, simular y subir

| Acción | Cómo |
| --- | --- |
| Compilar | PlatformIO: **Build** (✓) o `pio run` |
| Simular | Compilar y después **Wokwi: Start Simulator** (F1) |
| Subir a la placa | PlatformIO: **Upload** (→) o `pio run -t upload` |
| Monitor serie | PlatformIO: **Serial Monitor** o `pio device monitor` (115200 baudios) |

## Convenciones del código

- Nombres en español: clases en `PascalCase`, métodos y variables en `camelCase`, constantes en `MAYUSCULAS`.
- Una librería por carpeta en `lib/`, con su `.h` (*include guards*) y su `.cpp`.
- Cada clase arranca su hardware con un método `iniciar()` llamado desde el `setup()`, nunca en el constructor.
- Orden del `setup()`: Serial → hardware local → WiFi → servicios de red.
- Sin `delay()` en el `loop()`: tiempos con `millis()` y variables `unsigned long`.
- Pines, umbral e intervalos solo en `include/config.h`; credenciales solo en `include/secrets.h`.

## Integrantes

Grupo N.º __ · TA 4k01 · 2026

| Nombre | Legajo |
| --- | --- |
| | |

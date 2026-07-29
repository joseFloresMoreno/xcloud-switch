# Greenlight Switch

Cliente de **streaming local Xbox → Nintendo Switch** como homebrew, escrito en C/C++ con libnx.  
El Xbox y la Switch deben estar en la misma red Wi-Fi. No se requiere PC intermedio.

---

## Estado actual

### Lo que funciona hoy
- UI gráfica SDL2 con diseño estilo Moonlight (paleta Xbox verde, tarjetas, header/footer)
- Pantalla de lista de consolas Xbox (`HOST_LIST`) con selección por D-Pad
- Pantalla de conexión con spinner animado y estado de la sesión
- Pantalla de error con caja visual
- Módulo `xbox_host`: estructura de consolas descubiertas, entrada manual de IP
- API Xbox (`xcloud_api`) con llamadas HTTP reales vía libcurl (Bearer token, endpoints REST)
- Fuente del sistema Switch cargada automáticamente (sin archivos TTF externos)
- Compilación condicional: SDL2 cuando está disponible, consola printf como fallback

### Stubs pendientes (no funcional aún)
| Módulo | Bloqueado por |
|---|---|
| Descubrimiento automático de Xbox (mDNS) | Implementación UDP multicast pendiente |
| Autenticación OAuth Xbox Live | Flujo de token real pendiente |
| WebRTC (SDP/ICE) | Integración `libdatachannel` pendiente |
| Decodificación de audio | Integración `libopus` pendiente |
| Decodificación de video | Integración FFmpeg / H.264 pendiente |
| Pantalla STREAMING | Depende del pipeline de video |

---

## Arquitectura

```
src/
  main.cpp          — loop principal, navegación entre pantallas, gestión de subsistemas
  ui_sdl.cpp        — UI gráfica SDL2 (activa con HAVE_SDL2)
  ui.c              — UI consola fallback (printf, sin SDL2)
  xbox_host.c       — struct XboxHost, lista de consolas, entrada manual, stub mDNS
  xcloud_api.c      — cliente HTTP Xbox API (Bearer auth, HOME y CLOUD type)
  session.c         — struct XCloudSession, estados de sesión
  webrtc_client.c   — stub WebRTC (SDP/ICE placeholders)
  audio_decoder.c   — stub decodificador Opus
  video_decoder.c   — stub decodificador H.264
  input_handler.c   — inicialización del pad HID

include/
  ui.h              — interfaz UI (compatible C/C++ con extern "C" guards)
  xbox_host.h       — tipos XboxHost, XboxHostList, XboxHostState
  xcloud_api.h      — cliente API, tipos XCLOUD_TYPE_HOME / XCLOUD_TYPE_CLOUD
  session.h         — tipos de sesión y estados
  webrtc_client.h   — interfaz WebRTC
  audio_decoder.h   — interfaz decodificador de audio
  video_decoder.h   — interfaz decodificador de video
  input_handler.h   — interfaz de entrada
```

### Flujo de pantallas

```
HOST_LIST ──[A]──► CONNECTING ──(futuro)──► STREAMING
    ▲                  │
    └──────[B]─────────┘
```

---

## Requisitos

- **WSL2** con devkitPro instalado en `/opt/devkitpro`
- `switch-dev` (toolchain base): `sudo dkp-pacman -S switch-dev`

### Dependencias opcionales (se detectan automáticamente en CMake)

| Paquete | Habilita | Instalar |
|---|---|---|
| `switch-curl` | Llamadas HTTP reales a la API Xbox | `sudo dkp-pacman -S switch-curl` |
| `switch-sdl2` | UI gráfica (reemplaza consola) | `sudo dkp-pacman -S switch-sdl2` |
| `switch-sdl2_ttf` | Texto en la UI | `sudo dkp-pacman -S switch-sdl2_ttf` |
| `switch-freetype` | Motor de fuentes | `sudo dkp-pacman -S switch-freetype` |
| `switch-harfbuzz` | Shaping de texto (requerido por SDL2_ttf) | incluido con `switch-freetype` |

Instalar todo de una vez:
```bash
sudo dkp-pacman -S switch-curl switch-sdl2 switch-sdl2_ttf switch-freetype
```

---

## Compilar

**Siempre desde WSL2:**

```bash
export DEVKITPRO=/opt/devkitpro
cd '/mnt/c/Users/Jose_Flores/OneDrive - PROTIVITI DIGITAL LEARNING SOLUTIONS SPA/Documentos/PROYECTOS_VSC/GREENLIGHT_SWITCH'
./build.sh
```

El script detecta qué dependencias están instaladas y activa las funciones correspondientes.  
Resultado: `build/greenlight-switch.nro`

### CMake manual (alternativa)
```bash
export DEVKITPRO=/opt/devkitpro
mkdir -p build && cd build
cmake .. -DDEVKITPRO=$DEVKITPRO
DEVKITPRO=$DEVKITPRO make -j$(nproc)
```

---

## Instalar en Switch

1. Copia `build/greenlight-switch.nro` a `/switch/` en la SD
2. Lanza desde el Homebrew Launcher (HBL)

> Si la pantalla queda en negro: revisar el registro del CFW en la consola serial.

---

## Próximos pasos

### Inmediato
1. **mDNS** — implementar descubrimiento automático de Xbox en `src/xbox_host.c`  
   - Protocolo: UDP multicast `224.0.0.251:5353`, servicio `_xbox-smartglass._tcp.local`
   - Referencia: [xbox-smartglass-core-python](https://github.com/OpenXbox/xbox-smartglass-core-python)

2. **Autenticación** — obtener token Xbox Live real  
   - Flujo OAuth 2.0 / device flow de Microsoft
   - Reemplazar `PLACEHOLDER_TOKEN` en `main.cpp`

3. **Entrada manual de IP** — teclado en pantalla o archivo de config en la SD

### Siguiente fase
4. **WebRTC real** — integrar `libdatachannel` en `webrtc_client.c`
5. **Audio** — integrar `libopus` en `audio_decoder.c`
6. **Video** — integrar decodificador H.264 (FFmpeg port o similar) en `video_decoder.c`
7. **Pantalla STREAMING** — renderizar el video en SDL2 a pantalla completa

---

## Notas técnicas

- El orden de link es importante: `nx` (libnx) va al final para que `curl` y `SDL2` resuelvan sus símbolos de Switch OS
- `harfbuzz` es requerido por la versión moderna de `SDL2_ttf` para shaping de texto
- Los headers C (`ui.h`, `xbox_host.h`) usan guardas `#ifdef __cplusplus / extern "C"` para ser incluibles desde C++
- La fuente del sistema Switch se carga vía `plGetSharedFontByType` (no se necesita empaquetar TTF)

---

Este repositorio es la base para portar Greenlight a C++ nativo en Switch.

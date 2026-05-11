# Xbox xCloud Client para Nintendo Switch

**Cliente homebrew para streaming de Xbox Game Pass vía xCloud a Nintendo Switch**

## 📋 Estado del Proyecto

```
✅ Fase 1: Estructura Base & Build System - COMPLETADO
⏳ Fase 2: REST API & Autenticación - PENDIENTE
⏳ Fase 3: WebRTC Client - PENDIENTE
⏳ Fase 4: Decodificadores (H.264 + Opus) - PENDIENTE
⏳ Fase 5: Input/Output & UI - PENDIENTE
⏳ Fase 6: Integration & Flow - PENDIENTE
⏳ Fase 7: Testing & Optimización - PENDIENTE
```

**Versión actual:** 0.1 (Alpha - Estructura Base)  
**Última actualización:** 2026-05-11

---

## 🎮 Características Planeadas

### MVP (Mínimo Viable)
- ✅ Estructura modular y compilable
- ✅ Build system DevKitPro ready
- ✅ Executable .nro generado
- ⏳ Autenticación Xbox Live
- ⏳ Conexión a xCloud
- ⏳ Decodificación video H.264
- ⏳ Audio Opus
- ⏳ Control con Joy-Con

### Futuro
- Interfaz gráfica mejorada
- Múltiples sesiones simultáneas
- Estadísticas en tiempo real
- Soporte para chat de voz
- Integración con Home Console

---

## 🛠️ Instalación & Compilación

### Requisitos

- **DevKitPro** instalado (`/opt/devkitpro`)
- **devkitA64** (ARM64 toolchain para Switch)
- **CMake 3.15+**
- **GNU Make**

### Instalación rápida en WSL2/Linux

```bash
# 1. Instalar DevKitPro (si no lo tienes)
curl https://devkitpro.org/install | bash
dkp-pacman -S switch-dev

# 2. Clonar proyecto
git clone <repo-url> xcloud-switch
cd xcloud-switch

# 3. Compilar
export DEVKITPRO=/opt/devkitpro
mkdir build && cd build
cmake ..
make

# Salida:
# - xcloud-switch.elf (2.5MB, con debug)
# - xcloud-switch.nro (165KB, optimizado para Switch)
```

### Build automático

```bash
bash build.sh
```

---

## 📁 Estructura del Proyecto

```
xcloud-switch/
├── CMakeLists.txt              # Build configuration (DevKitPro + ARM64)
├── build.sh                    # Script de build automático
├── README.md                   # Este archivo
├── PROGRESS.md                 # Log de progreso del desarrollo
│
├── include/                    # Headers (interfaces)
│   ├── session.h              # Gestión de sesiones
│   ├── xcloud_api.h           # Cliente REST API Xbox
│   ├── webrtc_client.h        # WebRTC negotiation
│   ├── video_decoder.h        # H.264 decoder
│   ├── audio_decoder.h        # Opus decoder
│   ├── input_handler.h        # Joy-Con input
│   └── ui.h                   # UI/Rendering
│
├── src/                        # Implementaciones
│   ├── main.c                 # Entry point + main loop
│   ├── session.c              # Session management
│   ├── xcloud_api.c           # REST endpoints
│   ├── webrtc_client.c        # WebRTC peer connection
│   ├── video_decoder.c        # H.264 decoding (ffmpeg)
│   ├── audio_decoder.c        # Opus decoding (libopus)
│   ├── input_handler.c        # Joy-Con/HID input
│   └── ui.c                   # Console UI
│
├── build/                      # Build output (generado)
│   ├── xcloud-switch.elf      # Executable (debug)
│   ├── xcloud-switch.nro      # Homebrew executable (optimizado)
│   ├── control.nacp           # Metadata
│   └── CMakeFiles/            # CMake internals
│
└── docs/                       # Documentación adicional (futuro)
```

---

## 🔧 Arquitectura

```
┌─────────────────────────────────────────┐
│          Nintendo Switch                │
├─────────────────────────────────────────┤
│  main() - main loop, inicialización     │
├─────────────────────────────────────────┤
│  UI Layer (Console/Graphics)            │
├─────────────────────────────────────────┤
│  Input Handler (Joy-Con)                │
├─────────────────────────────────────────┤
│  Session Manager                        │
├─────────────────────────────────────────┤
│  WebRTC Client                          │
│  - PeerConnection                       │
│  - SDP Negotiation                      │
│  - ICE Candidates                       │
├─────────────────────────────────────────┤
│  Media Decoders                         │
│  - H.264 (ffmpeg)                       │
│  - Opus (libopus)                       │
├─────────────────────────────────────────┤
│  xCloud REST API Client (libcurl)       │
├─────────────────────────────────────────┤
│  HTTPS/TLS (openssl)                    │
└─────────────────────────────────────────┘
```

---

## 📊 Flujo de Conexión (Planeado)

```
1. Usuario inicia la app (HBL)
2. Ingresa token Microsoft
3. App conecta a servers Xbox xCloud
4. Negocia WebRTC (SDP + ICE)
5. Recibe stream H.264 + Opus
6. Decodifica video/audio
7. Renderiza en pantalla Switch
8. Procesa input Joy-Con
9. Envía comandos a Xbox
```

---

## 🎮 Cómo Usar (Cuando esté listo)

### Instalación en Switch

1. **Obtener el .nro:**
   ```bash
   # Después de compilar:
   cp build/xcloud-switch.nro /path/to/sd/switch/
   ```

2. **En Switch:**
   - Abre Homebrew Launcher
   - Busca "xCloud Switch"
   - Ejecuta

3. **En la app:**
   - Ingresa tu token de Xbox Live
   - Selecciona juego
   - ¡A jugar!

---

## 📝 Log de Desarrollo

### 2026-05-11 - Fase 1 Completada ✅

**Logros:**
- ✅ DevKitPro instalado (`/opt/devkitpro`)
- ✅ devkitA64 (ARM64) compilando
- ✅ Estructura modular de 16 archivos
- ✅ CMakeLists.txt funcional
- ✅ Primer ELF compilado (ARM32)
- ✅ ELF ARM64 generado (2.5MB)
- ✅ **.nro listo para Switch** (165KB)

**Próximos pasos:**
- Fase 2: Implementar REST API de Xbox xCloud
- Integrar libcurl para HTTPS
- Endpoints de autenticación y sesiones

Ver `PROGRESS.md` para detalles completos.

---

## 🔗 Referencias

### Tecnologías Usadas

| Componente | Librería | Versión |
|-----------|----------|---------|
| ARM64 Toolchain | devkitA64 | r15.2.0 |
| Nintendo Switch SDK | libnx | Latest |
| REST Client | libcurl | 7.x |
| WebRTC | libdatachannel | 0.x (pendiente) |
| Video Codec | ffmpeg | 5.x (pendiente) |
| Audio Codec | libopus | 1.x (pendiente) |
| Build System | CMake | 3.15+ |

### Documentación Externa

- [DevKitPro Docs](https://devkitpro.org)
- [libnx GitHub](https://github.com/switchbrew/libnx)
- [Greenlight (Reference)](https://github.com/unknownskl/greenlight)
- [Moonlight (Architecture Reference)](https://github.com/moonlight-stream/moonlight-common-c)
- [Xbox xCloud Protocol](https://github.com/unknownskl/greenlight) - Reverse-engineered

---

## 👨‍💻 Desarrollo

### Requisitos para Desarrollar

```bash
# Editar código
code xcloud-switch/

# Compilar en mi ambiente:
export DEVKITPRO=/opt/devkitpro
cd build
cmake ..
make

# Ver cambios en ejecución (cuando tenga emulador)
# (pendiente Ryujinx/emulador Switch)
```

### Estructura del Código

- **C99 standard** para compatibilidad
- **Headers modulares** - cada componente independiente
- **Funciones stub** - placeholders para Fase 2+
- **Logging simple** - `printf()` a console

### Agregar Características

1. Edita el header (`include/`)
2. Implementa en `.c` (`src/`)
3. Prueba compilación: `cd build && cmake .. && make`

---

## ⚙️ Compilación Avanzada

### Variables de Entorno

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:$PATH
```

### Compilación Manual

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4  # 4 jobs paralelos

# Crear .nro manualmente:
$DEVKITPRO/tools/bin/nacptool --create "xCloud" "Autor" "0.1" control.nacp
$DEVKITPRO/tools/bin/elf2nro xcloud-switch.elf xcloud-switch.nro --nacp=control.nacp
```

### Debug

```bash
# Incluye símbolos de debug en ELF
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Ver símbolos:
aarch64-none-elf-nm xcloud-switch.elf | grep main
```

---

## 🐛 Troubleshooting

### Error: `aarch64-none-elf-gcc: command not found`

```bash
# Solución:
export PATH=/opt/devkitpro/devkitA64/bin:$PATH
which aarch64-none-elf-gcc
```

### Error: `DEVKITPRO not found`

```bash
# Solución:
export DEVKITPRO=/opt/devkitpro
cmake ..
```

### Error: `cannot find -lnx`

```bash
# Solución: Compilar sin libnx (por ahora)
# Edita CMakeLists.txt y remueve 'nx' de target_link_libraries
```

---

## 📜 Licencia & Disclaimer

**Este proyecto es educativo y de reverse-engineering.**

- **NO AFILIADO** con Microsoft, Xbox o Nintendo
- **SOLO PARA USO PERSONAL** en consolas que poseas
- **Game Pass** requiere suscripción válida
- Usa bajo **tu propio riesgo**

Respeta los términos de servicio de Xbox Live y Nintendo.

---

## 👤 Autor

**Jose Flores Moreno**  
Diseñador gráfico + Estudiante Programación  
Santiago, Chile

---

## 📞 Contacto & Soporte

Para problemas con:
- **DevKitPro:** [DevKitPro Discord](https://discord.gg/devkitpro)
- **libnx:** [Switchbrew GitHub](https://github.com/switchbrew)
- **Este proyecto:** Ver `PROGRESS.md` para estado actual

---

**Última actualización:** 2026-05-11  
**Estado:** 🟢 Fase 1 Completada - .nro Generado  
**Próximo hito:** Fase 2 - REST API

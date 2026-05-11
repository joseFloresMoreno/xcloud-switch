# GREENLIGHT_SWITCH - Xbox xCloud Cliente para Nintendo Switch

## 📦 Contenido de la Carpeta

### Raíz
- **README.md** - Documentación completa del proyecto
- **CMakeLists.txt** - Configuración de build (DevKitPro + ARM64)
- **build.sh** - Script de compilación automática
- **PROGRESS.md** - Log de progreso del desarrollo

### /build/
**Salida de compilación**
- `xcloud-switch.elf` - Executable ARM64 (2.5MB, con debug)
- `xcloud-switch.nro` - **¡¡LISTO PARA SWITCH!!** (165KB, optimizado)
- `control.nacp` - Metadata (título, autor, icono)
- `CMakeFiles/` - Internals de cmake

### /src/
**Código fuente (8 archivos)**
- `main.c` - Entry point y main loop
- `session.c` - Gestión de sesiones
- `xcloud_api.c` - Cliente REST API Xbox
- `webrtc_client.c` - WebRTC peer connection (stub)
- `video_decoder.c` - H.264 decoder (stub)
- `audio_decoder.c` - Opus decoder (stub)
- `input_handler.c` - Joy-Con input
- `ui.c` - Console UI

### /include/
**Headers (7 archivos)**
- `session.h`, `xcloud_api.h`, `webrtc_client.h`
- `video_decoder.h`, `audio_decoder.h`, `input_handler.h`, `ui.h`

### /docs/
Documentación adicional (pendiente)

### /lib/
Librerías externas (pendiente)

---

## 🚀 Quick Start

### Compilar desde cero

```bash
cd GREENLIGHT_SWITCH
bash build.sh
```

O manualmente:
```bash
export DEVKITPRO=/opt/devkitpro
mkdir build && cd build
cmake ..
make
```

### Instalar en Switch

1. Conecta SD card a PC
2. Copia `build/xcloud-switch.nro` a `/switch/` en la SD
3. Inserta SD en Switch
4. Abre Homebrew Launcher → Busca "xCloud Switch"

---

## 📊 Estado Actual

**Fase 1: Completada ✅**
- Estructura modular creada
- Build system funcional
- DevKitPro setup correcto
- **.nro generado y listo**

**Fases Pendientes (2-7):**
- REST API & Autenticación
- WebRTC Client
- Decodificadores (H.264 + Opus)
- Input/Output
- UI mejorada
- Testing

---

## 📝 Notas Importantes

- Este es un **proyecto educativo** - reverse-engineering de Greenlight
- Requiere **Game Pass** válido en Xbox
- **NO AFILIADO** con Microsoft/Xbox/Nintendo
- Solo para **uso personal** en consolas que poseas

---

## 🔧 Requisitos para Compilar

- DevKitPro instalado (`/opt/devkitpro`)
- devkitA64 (ARM64 toolchain)
- CMake 3.15+
- GNU Make
- Linux/WSL2 (recomendado)

---

## 📂 Estructura Completa

```
GREENLIGHT_SWITCH/
├── README.md (documentación principal)
├── CMakeLists.txt (build config)
├── build.sh (script auto)
├── PROGRESS.md (log desarrollo)
│
├── src/ (código fuente - 8 archivos .c)
├── include/ (headers - 7 archivos .h)
│
├── build/ (output - generado al compilar)
│   ├── xcloud-switch.elf ✅
│   ├── xcloud-switch.nro ✅ ← INSTALA ESTO EN SWITCH
│   └── control.nacp ✅
│
├── docs/ (documentación extra)
└── lib/ (librerías externas)
```

---

## ✅ Checklist para Desarrollo

- [x] Estructura modular
- [x] CMakeLists.txt ARM64
- [x] Headers diseñados
- [x] Código skeleton creado
- [x] Compilación exitosa
- [x] .nro generado
- [ ] REST API implementada
- [ ] WebRTC integrado
- [ ] Decodificadores listos
- [ ] Testing en Switch

---

**Última actualización:** 2026-05-11  
**Próximo paso:** Fase 2 - Implementar REST API

# 📖 ÍNDICE DEL PROYECTO

## 🎯 COMIENZA AQUÍ

1. **START_HERE.md** ← Lee esto primero
2. **README.md** ← Documentación completa
3. **PROGRESS.md** ← Log de desarrollo

## 📁 ESTRUCTURA

```
GREENLIGHT_SWITCH/
│
├── 📄 START_HERE.md          ← PUNTO DE ENTRADA (Lee primero)
├── 📄 README.md              ← Documentación principal
├── 📄 PROGRESS.md            ← Log de desarrollo
├── 📄 SYNC_STATUS.txt        ← Estado de sincronización
├── 📄 CMakeLists.txt         ← Configuración de build
├── 📄 build.sh               ← Script automático de compilación
│
├── 📁 src/                   ← CÓDIGO FUENTE
│   ├── main.c               (entry point + main loop)
│   ├── session.c            (gestión de sesiones)
│   ├── xcloud_api.c         (REST API - PRÓXIMO)
│   ├── webrtc_client.c      (WebRTC - FUTURO)
│   ├── video_decoder.c      (H.264 - FUTURO)
│   ├── audio_decoder.c      (Opus - FUTURO)
│   ├── input_handler.c      (Joy-Con input)
│   └── ui.c                 (interfaz usuario)
│
├── 📁 include/               ← HEADERS (INTERFACES)
│   ├── session.h
│   ├── xcloud_api.h
│   ├── webrtc_client.h
│   ├── video_decoder.h
│   ├── audio_decoder.h
│   ├── input_handler.h
│   └── ui.h
│
├── 📁 build/                 ← OUTPUT (GENERADO)
│   ├── 🎮 xcloud-switch.nro  ← ¡¡INSTALA ESTO EN SWITCH!!
│   ├── xcloud-switch.elf     (executable con debug)
│   └── control.nacp          (metadata)
│
├── 📁 docs/                  ← Documentación extra
└── 📁 lib/                   ← Librerías externas
```

## 🔴 ARCHIVOS CRÍTICOS

### Para compilar:
- **CMakeLists.txt** - Build configuration
- **src/*.c** - Código fuente
- **include/*.h** - Headers

### Para instalar en Switch:
- **build/xcloud-switch.nro** ← ESTE ES EL QUE NECESITAS

## 🟢 CÓMO USAR

### 1. Compilar
```bash
cd GREENLIGHT_SWITCH
bash build.sh
```

### 2. Instalar en Switch
Copia el archivo:
```
build/xcloud-switch.nro → /switch/ (en SD card)
```

### 3. Ejecutar
- Abre Homebrew Launcher en Switch
- Busca "xCloud Switch"
- ¡A jugar!

## 📊 ESTADO

| Fase | Descripción | Estado |
|------|-------------|--------|
| 1 | Estructura Base | ✅ COMPLETADA |
| 2 | REST API | ⏳ EN DESARROLLO |
| 3 | WebRTC | ⏳ PENDIENTE |
| 4 | Decodificadores | ⏳ PENDIENTE |
| 5 | Input/Output | ⏳ PENDIENTE |
| 6 | UI Completa | ⏳ PENDIENTE |
| 7 | Testing | ⏳ PENDIENTE |

## 🚀 PRÓXIMOS PASOS

**Fase 2: REST API & Autenticación**
- Implementar endpoints Xbox xCloud
- Autenticación Microsoft
- Gestión de sesiones
- Control.nacp device spoofing

**Ubicación:** Jose trabajará en `/home/jose_flores/.openclaw/workspace/xcloud-switch/`

**Sincronización:** Se copiará a Windows cuando solicitees

## 🎮 REQUISITOS PARA SWITCH

- Nintendo Switch con Homebrew Launcher habilitado
- Game Pass válido en Xbox Live
- SD card con espacio libre
- Archivo .nro en `/switch/`

## 📝 LICENCIA

- Proyecto educativo - Reverse-engineering
- NO AFILIADO con Microsoft/Xbox/Nintendo
- Solo uso personal
- Respeta términos de servicio

## 🔗 REFERENCIAS ÚTILES

- [DevKitPro](https://devkitpro.org)
- [libnx (Switch SDK)](https://github.com/switchbrew/libnx)
- [Greenlight (Referencia)](https://github.com/unknownskl/greenlight)
- [Moonlight (Arquitectura)](https://github.com/moonlight-stream/moonlight-common-c)

---

**Última actualización:** 2026-05-11  
**Versión:** 0.1 Alpha  
**Status:** 🟢 Sincronizado y listo para continuación

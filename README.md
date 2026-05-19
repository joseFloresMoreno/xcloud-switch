# GREENLIGHT_SWITCH

Minimal Nintendo Switch homebrew skeleton extracted from the Greenlight project.

Estado
- Proyecto reseteado a un `hello-switch` mínimo (build/hello-switch.nro disponible).
- Archivos y extractos de Greenlight se conservan en `src/` y `include/` para futura migración.

Requisitos
- WSL2 (recomendada para compilación)
- devkitPro instalado en WSL (ej. `/opt/devkitpro`)
- `cmake`, `make`, `elf2nro`, `nacptool` (vienen con devkitPro)

Flujo de trabajo (resumen)
1. Siempre compilar desde WSL (exportar `DEVKITPRO`).
2. Copiar los artefactos resultantes (`.nro`, `.elf`) a Windows para transferir al SD de la Switch.

Compilar (WSL)
```bash
export DEVKITPRO=/opt/devkitpro
cd '/mnt/c/Users/Jose_Flores/OneDrive - PROTIVITI DIGITAL LEARNING SOLUTIONS SPA/Documentos/PROYECTOS_VSC/GREENLIGHT_SWITCH'
./build.sh
```

Si prefieres usar CMake manualmente:
```bash
export DEVKITPRO=/opt/devkitpro
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Copiar artefactos a Windows (desde WSL)
```bash
cp build/hello-switch.nro /mnt/c/Users/Jose_Flores/OneDrive\ -\ PROTIVITI\ DIGITAL\ LEARNING\ SOLUTIONS\ SPA/Documentos/PROYECTOS_VSC/GREENLIGHT_SWITCH/build/
```

Probar en Switch
- Copia `build/hello-switch.nro` a la carpeta `/switch/` de la SD y ejecútalo.

Próximos pasos (plan corto)
- Crear un `hello` usando Plutonium (renderer SDL2/Plutonium).
- Diseñar estructura C++ para migrar los módulos de Greenlight.

Notas
- Si la pantalla en la Switch queda en negro con los binarios mínimos, puede ser un problema de CFW/libnx o del flujo de carga; revisar en la consola serial/registro del CFW.
- Mantén siempre el entorno de compilación en WSL y copia solo los artefactos a Windows.

---
Este repositorio se mantiene como base para portar Greenlight a C++ y Plutonium.

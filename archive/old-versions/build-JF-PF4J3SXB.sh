#!/bin/bash
set -e

# Resolve devkitPro: prefer existing DEVKITPRO env, then auto-detect
if [ ! -d "${DEVKITPRO:-}" ]; then
    if [ -d "/opt/devkitpro" ]; then
        DEVKITPRO=/opt/devkitpro          # WSL2 / Linux — full portlibs (curl, SDL2)
    elif [ -d "/c/devkitpro" ]; then
        DEVKITPRO=/c/devkitpro            # Windows MSYS2 — compiler only, no portlibs
    else
        echo "ERROR: devkitPro not found. Set DEVKITPRO or install from https://devkitpro.org"
        exit 1
    fi
fi

DEVKITA64="$DEVKITPRO/devkitA64"
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

if [ ! -d "$DEVKITA64" ]; then
    echo "ERROR: DEVKITA64 not found at $DEVKITA64"
    echo "Install with: dkp-pacman -S switch-dev"
    exit 1
fi

# On Windows (MSYS2): warn that portlibs are likely missing
if [ -d "$DEVKITPRO/msys2" ] && [ ! -f "$DEVKITPRO/portlibs/switch/lib/libcurl.a" ]; then
    echo "WARNING: switch-curl/SDL2 not installed in this devkitPro."
    echo "  For the full build (curl + SDL2), run this script from WSL2:"
    echo "    wsl bash build.sh"
    echo "  Or install via: sudo dkp-pacman -S switch-curl switch-sdl2 switch-sdl2_ttf switch-freetype"
    echo ""
fi

echo "=============================================="
echo "  Greenlight Switch Builder"
echo "=============================================="
echo "DEVKITPRO : $DEVKITPRO"
echo "Project   : $PROJECT_DIR"
echo "Build dir : $BUILD_DIR"
echo ""

export DEVKITPRO
export PATH="$DEVKITA64/bin:$PATH"

if [ -d "$DEVKITPRO/msys2" ]; then
    # Windows MSYS2: GCC needs TEMP set and cmake run inside MSYS2 shell
    MSYS2_SHELL="$DEVKITPRO/msys2/usr/bin/bash.exe"
    DKP="$DEVKITPRO"
    PRJ="$PROJECT_DIR"
    BLD="$BUILD_DIR"
    echo "[1/2] Configuring + Building (via MSYS2)..."
    "$MSYS2_SHELL" -c "
        export DEVKITPRO=$DKP
        export PATH=$DKP/devkitA64/bin:/usr/bin:\$PATH
        export TEMP=/tmp TMP=/tmp TMPDIR=/tmp
        rm -rf \"$BLD\" && mkdir -p \"$BLD\"
        cmake.exe -S\"$PRJ\" -B\"$BLD\" -DDEVKITPRO=$DKP
        JOBS=\$(nproc 2>/dev/null || echo 4)
        make -C \"$BLD\" -j\$JOBS
    "
    NRO_PATH="$BUILD_DIR/greenlight-switch.nro"
else
    # WSL2 / Linux: standard cmake + make
    echo "[1/2] Configuring..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DDEVKITPRO="$DEVKITPRO"
    echo "[2/2] Building..."
    make -C "$BUILD_DIR" -j"$(nproc)"
    NRO_PATH="$BUILD_DIR/greenlight-switch.nro"
fi

echo ""
echo "=============================================="
echo "  Build Complete"
echo "=============================================="
echo "  NRO: $NRO_PATH"
echo ""
echo "Install: copy build/greenlight-switch.nro to /switch/ on SD card"
echo ""
